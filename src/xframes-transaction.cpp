#include <xframes-transaction.h>
#include <xframes.h>
#include <implot_renderer.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>
#include <implot.h>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif

class HostedRenderer;
struct WindowState {
    std::mutex gate;
    std::condition_variable changed;
    std::deque<json> events;
    std::deque<json> commands;
    std::thread thread;
    std::unique_ptr<HostedRenderer> renderer;
    // The engine this window renders. Events carry its visible revision: the tree the
    // frame raising them drew (XFrames::VisibleRevision).
    XFrames* runtime = nullptr;
    std::string title;
    std::string error;
    bool ready = false;
    bool done = false;
    bool safe = false;
    bool wakeEnabled = false;
    bool closeRequested = false;
    bool destroyRequested = false;
    bool overflow = false;
    std::string handle = "0";
    bool diagnostics = false;
    json input = json::object();
    uint64_t widgetEvents = 0;
#ifdef XFRAMES_TEST_HOOKS
    int failInit = 0;
    bool failRender = false;
    bool failDestroy = false;
#endif
    ~WindowState();

    void Event(const char* kind, int id = 0, json payload = nullptr) noexcept {
        try {
            const std::lock_guard lock(gate);
            if (events.size() >= 4096) {
                overflow = true;
                closeRequested = true;
                if (wakeEnabled) glfwPostEmptyEvent();
                return;
            }
            events.push_back({{"kind", kind}, {"nodeId", id}, {"payload", std::move(payload)},
                {"occurredAtMs", XFrames::DiagnosticsNowMs()},
                {"revision", std::to_string(runtime ? runtime->VisibleRevision() : 0)}});
            if (id != 0) ++widgetEvents;
        } catch (...) {
            const std::lock_guard lock(gate);
            overflow = true;
            closeRequested = true;
            if (wakeEnabled) glfwPostEmptyEvent();
        }
    }
    void Close() {
        const std::lock_guard lock(gate);
        closeRequested = true;
        if (wakeEnabled) glfwPostEmptyEvent();
    }
    void Destroy() {
        const std::lock_guard lock(gate);
        closeRequested = true;
        destroyRequested = true;
        changed.notify_all();
        if (wakeEnabled) glfwPostEmptyEvent();
    }
};

struct xf_surface {
    XFrames runtime{"xframes-shared", std::nullopt};
    std::string result;
    std::unique_ptr<WindowState> window;
};

namespace {
thread_local std::string lastError;
thread_local WindowState* activeWindow = nullptr;
std::atomic<bool> windowLease{false}; // ImGui/GLFW process globals: one rendered surface.
#ifdef XFRAMES_TEST_HOOKS
std::atomic<int> nextInitFailure{0};
std::atomic<int64_t> guiAllocations{0};
std::mutex testSurfaceGate;
xf_surface* testSurface = nullptr;
bool trackingAllocations = false;
void* trackedAllocate(size_t size, void*) {
    auto* memory = std::malloc(size);
    if (memory) ++guiAllocations;
    return memory;
}
void trackedFree(void* memory, void*) {
    if (memory) --guiAllocations;
    std::free(memory);
}
#endif

struct WindowOptions {
    std::string title = "XFrames";
    int width = 640, height = 420;
    bool visible = true, diagnostics = false;
    explicit WindowOptions(const char* wire) {
        const auto input = json::parse(wire ? wire : "{}");
        if (!input.is_object()) throw std::invalid_argument("Window options must be an object");
        for (const auto* name : {"width", "height"}) {
            if (input.contains(name) && (!input[name].is_number_integer() || input[name] < 1 || input[name] > 16384))
                throw std::invalid_argument("Window dimensions must be integers from 1 to 16384");
        }
        // Parse everything before taking the window lease or creating either graphics context.
        title = input.value("title", title);
        width = input.value("width", width);
        height = input.value("height", height);
        visible = input.value("visible", visible);
        diagnostics = input.value("diagnostics", diagnostics);
    }
};

// Upstream renderer constructors allocate contexts before the complete renderer exists.
// Until unique_ptr owns it, clean up any contexts left by a throwing constructor.
struct ContextConstructionGuard {
    ImGuiContext* gui = ImGui::GetCurrentContext();
    ImPlotContext* plot = ImPlot::GetCurrentContext();
    bool committed = false;
    ~ContextConstructionGuard() {
        if (!committed) {
            if (auto* current = ImPlot::GetCurrentContext(); current && current != plot) ImPlot::DestroyContext(current);
            if (auto* current = ImGui::GetCurrentContext(); current && current != gui) ImGui::DestroyContext(current);
            ImPlot::SetCurrentContext(plot);
            ImGui::SetCurrentContext(gui);
        }
    }
};

void installCallbacks(XFrames& runtime) {
    runtime.SetEventHandlers(
        [] {},
        [](int id, const std::string& v) { activeWindow->Event("textChanged", id, v); },
        [](int id, int v) { activeWindow->Event("comboChanged", id, v); },
        [](int id, float v) { activeWindow->Event("numberChanged", id, v); },
        [](int id, const float* values, int count) {
            activeWindow->Event("numbersChanged", id, std::vector<float>(values, values + count));
        },
        [](int id, bool v) { activeWindow->Event("booleanChanged", id, v); },
        [](int id) { activeWindow->Event("click", id); },
        [](int id, int column, int direction) { activeWindow->Event("tableSort", id, {{"column", column}, {"direction", direction}}); },
        [](int id, int column, const std::string& value) { activeWindow->Event("tableFilter", id, {{"column", column}, {"value", value}}); },
        [](int id, int row) { activeWindow->Event("tableRowClick", id, row); },
        [](int id, int row, const std::string& action) { activeWindow->Event("tableAction", id, {{"row", row}, {"action", action}}); },
        [](int id, int completed, int total) { activeWindow->Event("prefetchProgress", id, {{"completed", completed}, {"total", total}}); },
        [](int id, const std::string& error) { activeWindow->Event("scriptError", id, error); });
    runtime.SetTableRangeHandler([](int id, int first, int last) {
        activeWindow->Event("tableRange", id, {{"first", first}, {"last", last}});
    });
}

template<class F> const char* jsonCall(xf_surface* surface, F action) noexcept {
    try {
        if (!surface) throw std::invalid_argument("Null native surface");
        surface->result = action(surface->runtime).dump();
        return surface->result.c_str();
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Unknown native exception"; }
    return nullptr;
}
}

// The production frame scheduler constructs/submits frames. This host additionally drains
// C-ABI commands on the window's owner thread before entering GLFW's queued-event wait.
class HostedRenderer final : public ImPlotRenderer {
    WindowState& state;
    bool visible;
public:
    HostedRenderer(XFrames& runtime, WindowState& state, const WindowOptions& options, std::string& fonts)
        : ImPlotRenderer(&runtime, "xframes-shared", state.title.c_str(), fonts, std::nullopt),
          state(state), visible(options.visible) {
        m_window_width = options.width;
        m_window_height = options.height;
#ifdef XFRAMES_TEST_HOOKS
        if (state.failInit == 1) throw std::runtime_error("Injected initialization failure after contexts");
#endif
    }
    void SetUp() override {
        if (!glfwInit()) throw std::runtime_error("GLFW initialization failed");
        glfwWindowHint(GLFW_VISIBLE, visible ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        m_glfwWindow = glfwCreateWindow(m_window_width, m_window_height, state.title.c_str(), nullptr, nullptr);
        if (!m_glfwWindow) throw std::runtime_error("GLFW window creation failed");
#ifdef XFRAMES_TEST_HOOKS
        if (state.failInit == 2) throw std::runtime_error("Injected initialization failure after window creation");
#endif
        InstallWindowCallbacks();
        glfwMakeContextCurrent(m_glfwWindow);
        glfwSwapInterval(1);
        SetCurrentContext();
        if (!ImGui_ImplGlfw_InitForOpenGL(m_glfwWindow, true) || !ImGui_ImplOpenGL3_Init("#version 130"))
            throw std::runtime_error("ImGui backend initialization failed");
#ifdef XFRAMES_TEST_HOOKS
        if (state.failInit == 3) throw std::runtime_error("Injected initialization failure after backends");
#endif
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        m_loadedFonts.push_back(io.Fonts->AddFontDefault()); // Embedded font; no working-directory assets.
        io.Fonts->Build();
        SetFontDefault(0);
        const std::lock_guard lock(state.gate);
        state.wakeEnabled = true;
#ifdef _WIN32
        state.handle = std::to_string(reinterpret_cast<uintptr_t>(glfwGetWin32Window(m_glfwWindow)));
#endif
    }
    void CleanUp() override {
        { const std::lock_guard lock(state.gate); state.wakeEnabled = false; }
        ImPlotRenderer::CleanUp();
    }
    void Run() {
        SetUp();
        m_loopRunning = true;
        const xframes::FrameReason reasons[] = {xframes::FrameReason::Layout, xframes::FrameReason::Interaction,
            xframes::FrameReason::Cursor, xframes::FrameReason::KeyRepeat, xframes::FrameReason::Hover};
        for (size_t i = 0; i < m_imguiActivity.size(); ++i)
            m_imguiActivity[i] = m_xframes->m_frameScheduler.Register(reasons[i]);
        StartScreenshotRequests();
        m_xframes->m_frameScheduler.AttachWake(WakeRenderer, this);
        m_xframes->Init(this);
        { const std::lock_guard lock(state.gate); state.ready = true; state.changed.notify_all(); }
        state.Event("ready");
        while (!glfwWindowShouldClose(m_glfwWindow)) {
            glfwPollEvents();
            std::deque<json> commands;
            bool close;
            { const std::lock_guard lock(state.gate); commands.swap(state.commands); close = state.closeRequested; }
#ifdef XFRAMES_TEST_HOOKS
            {
                const std::lock_guard lock(state.gate);
                if (state.failRender) { state.failRender = false; throw std::runtime_error("Injected rendering failure"); }
            }
#endif
            if (close) glfwSetWindowShouldClose(m_glfwWindow, GLFW_TRUE);
            for (const auto& command : commands) {
                const auto op = command.at("op").get<std::string>();
                if (op == "resize") SetWindowSize(command.at("width").get<int>(), command.at("height").get<int>());
                if (op == "screenshot") {
                    const auto path = command.at("path").get<std::string>();
                    RequestScreenshot(path, [this, path](std::optional<std::string> error) {
                        state.Event("screenshot", 0, {{"path", path}, {"error", error ? json(*error) : json(nullptr)}});
                    });
                }
            }
            ProcessWindowRequests();
            const bool available = UpdateSurfaceAvailability();
            if (glfwWindowShouldClose(m_glfwWindow)) break;
            const auto next = m_xframes->m_frameScheduler.TakeOpportunity();
            if (next.render) {
                DrawFrame();
                if (state.diagnostics) {
                    const std::lock_guard lock(state.gate);
                    state.input = {{"x", m_imGuiCtx->IO.MousePos.x}, {"y", m_imGuiCtx->IO.MousePos.y},
                        {"down", m_imGuiCtx->IO.MouseDown[0]}, {"active", m_imGuiCtx->ActiveId},
                        {"hovered", m_imGuiCtx->HoveredId}};
                }
            }
            else {
                if (next.terminal) {
                    state.Event("rendererError", 0, "Native renderer entered a terminal state");
                    break;
                }
                if (!available) FailScreenshotRequests("Framebuffer is unavailable");
                if (!next.deadline) m_resumingFromIdle = true;
                if (next.deadline) {
                    const double seconds = std::chrono::duration<double>(*next.deadline - m_xframes->m_frameScheduler.Now()).count();
                    if (seconds > 0) glfwWaitEventsTimeout(std::max(seconds, 0.001));
                } else glfwWaitEvents();
            }
        }
        // OS close first asks the managed owner to drain publication. Retain the surface
        // until xf_destroy establishes that boundary; do not dispose beneath an active host.
        { const std::lock_guard lock(state.gate); state.closeRequested = true; }
        state.Event("closeRequested");
        { std::unique_lock lock(state.gate); state.changed.wait(lock, [&] { return state.destroyRequested; }); }
        StopScreenshotRequests("Window closed before capture");
        CleanUp();
    }
};

WindowState::~WindowState() = default;

unsigned xf_abi_version() { return 1; }
const char* xf_last_error() { return lastError.c_str(); }
xf_surface* xf_create() {
    try { return new xf_surface(); }
    catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Native initialization failed"; }
    return nullptr;
}
xf_surface* xf_create_window(const char* wire) {
#ifndef _WIN32
    lastError = "Rendered C-ABI hosting currently supports Windows only";
    return nullptr;
#else
    bool leased = false;
    try {
        const WindowOptions options(wire);
        if (windowLease.exchange(true)) throw std::runtime_error("Only one rendered surface is supported per process");
        leased = true;
        auto surface = std::make_unique<xf_surface>();
        surface->window = std::make_unique<WindowState>();
        auto& state = *surface->window;
        state.runtime = &surface->runtime;
        state.title = options.title;
        state.diagnostics = options.diagnostics;
#ifdef XFRAMES_TEST_HOOKS
        state.failInit = nextInitFailure.exchange(0);
#endif
        surface->runtime.SetDiagnosticsEnabled(options.diagnostics);
        auto* ptr = surface.get();
        state.thread = std::thread([ptr, options] {
            auto& state = *ptr->window;
            activeWindow = &state;
            try {
                installCallbacks(ptr->runtime);
                std::string fonts = "{}";
                {
                    ContextConstructionGuard guard;
                    state.renderer = std::make_unique<HostedRenderer>(ptr->runtime, state, options, fonts);
                    guard.committed = true;
                }
                state.renderer->Run();
                state.safe = true;
            } catch (const std::exception& error) { state.error = error.what(); }
            catch (...) { state.error = "Unknown renderer failure"; }
            if (!state.error.empty() && state.ready) {
                // Rendering failures obey the same owner-drain boundary as OS close.
                // Publication can still be in flight when a frame throws.
                state.Event("rendererError", 0, state.error);
                std::unique_lock lock(state.gate);
                state.closeRequested = true;
                state.changed.wait(lock, [&] { return state.destroyRequested; });
            }
            if (!state.safe) {
                try {
                    if (state.renderer) state.renderer->CleanUp(); else ptr->runtime.Dispose();
                    state.safe = true;
                } catch (...) { state.error += "; native cleanup failed"; }
            }
            if (!state.error.empty()) state.Event("rendererError", 0, state.error);
            state.Event("closed");
            { const std::lock_guard lock(state.gate); state.done = true; state.changed.notify_all(); }
            activeWindow = nullptr;
        });
        { std::unique_lock lock(state.gate); state.changed.wait(lock, [&] { return state.ready || state.done; }); }
        if (!state.ready) {
            state.thread.join();
            throw std::runtime_error(state.error);
        }
#ifdef XFRAMES_TEST_HOOKS
        { const std::lock_guard lock(testSurfaceGate); testSurface = surface.get(); }
#endif
        return surface.release();
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Window initialization failed"; }
    if (leased) windowLease = false;
    return nullptr;
#endif
}
const char* xf_apply(xf_surface* surface, const char* wire) {
    if (!wire) { lastError = "Null commit payload"; return nullptr; }
    return jsonCall(surface, [wire](XFrames& runtime) { return runtime.ApplyCommit(wire).ToJson(); });
}
int xf_element_op(xf_surface* surface, int id, const char* operation) {
    try {
        if (!surface || !operation) throw std::invalid_argument("Surface and operation required");
        // The engine ignores malformed operations and unknown nodes; report them instead.
        const auto parsed = json::parse(operation);
        if (!parsed.is_object() || !parsed.contains("op") || !parsed["op"].is_string())
            throw std::invalid_argument("Operation must be an object with a string op");
        if (!surface->runtime.IsElementAlive(id)) throw std::invalid_argument("Unknown node or quarantined surface");
        std::string payload(operation);
        surface->runtime.QueueElementInternalOp(id, payload);
        return 1;
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Element operation failed"; }
    return 0;
}
const char* xf_state(xf_surface* surface) {
    return jsonCall(surface, [](XFrames& runtime) { return runtime.GetCommitState(); });
}
const char* xf_diagnostics(xf_surface* surface) {
    return jsonCall(surface, [](XFrames& runtime) { return runtime.GetDiagnosticsState(); });
}
const char* xf_frame(xf_surface* surface) {
    return jsonCall(surface, [surface](XFrames& runtime) {
        auto frame = runtime.GetDiagnosticsFrame();
        if (surface->window) {
            const std::lock_guard lock(surface->window->gate);
            frame["window"] = {{"ready", surface->window->ready}, {"closed", surface->window->done},
                {"closing", surface->window->closeRequested},
                {"handle", surface->window->done ? "0" : surface->window->handle},
                {"widgetEvents", surface->window->widgetEvents}, {"input", surface->window->input}};
        }
        return frame;
    });
}
const char* xf_poll_events(xf_surface* surface) {
    return jsonCall(surface, [surface](XFrames&) {
        auto events = json::array();
        if (!surface->window) return events;
        const std::lock_guard lock(surface->window->gate);
        for (auto& event : surface->window->events) events.push_back(std::move(event));
        surface->window->events.clear();
        if (surface->window->overflow) {
            events.push_back({{"kind", "rendererError"}, {"nodeId", 0}, {"payload", "Native event queue overflow"},
                {"occurredAtMs", XFrames::DiagnosticsNowMs()},
                {"revision", std::to_string(surface->runtime.VisibleRevision())}});
            surface->window->overflow = false;
        }
        return events;
    });
}
int xf_window_command(xf_surface* surface, const char* wire) {
    try {
        if (!surface || !surface->window || !wire) throw std::invalid_argument("Rendered surface and command required");
        const auto command = json::parse(wire);
        const auto op = command.at("op").get<std::string>();
        if (op == "close") { surface->window->Close(); return 1; }
        if (op == "resize") {
            const auto width = command.at("width").get<int>(), height = command.at("height").get<int>();
            if (width < 0 || height < 0 || width > 16384 || height > 16384) throw std::invalid_argument("Invalid window dimensions");
        } else if (op == "screenshot") { (void)command.at("path").get<std::string>(); }
        else throw std::invalid_argument("Unknown window command");
        const std::lock_guard lock(surface->window->gate);
        if (surface->window->done || surface->window->closeRequested) throw std::runtime_error("Window is closing");
        if (surface->window->commands.size() >= 256) throw std::runtime_error("Window command queue full");
        surface->window->commands.push_back(command);
        if (surface->window->wakeEnabled) glfwPostEmptyEvent();
        return 1;
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Window command failed"; }
    return 0;
}
int xf_destroy(xf_surface* surface) {
    try {
#ifdef XFRAMES_TEST_HOOKS
        const std::lock_guard testLock(testSurfaceGate);
#endif
        const bool rendered = surface && surface->window;
        if (rendered) {
#ifdef XFRAMES_TEST_HOOKS
            {
                const std::lock_guard lock(surface->window->gate);
                if (surface->window->failDestroy) {
                    surface->window->failDestroy = false;
                    throw std::runtime_error("Injected native teardown failure before release");
                }
            }
#endif
            surface->window->Destroy();
            if (surface->window->thread.joinable()) surface->window->thread.join();
            if (!surface->window->safe) throw std::runtime_error("Renderer teardown did not establish safe release");
        } else if (surface) surface->runtime.Dispose();
#ifdef XFRAMES_TEST_HOOKS
        if (surface == testSurface) testSurface = nullptr;
#endif
        delete surface;
        if (rendered) windowLease = false;
        return 1;
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Native teardown failed"; }
    return 0;
}

#ifdef XFRAMES_TEST_HOOKS
// Not declared in the public header and not exported by production builds.
extern "C" XF_API int xf_test_control(xf_surface* surface, const char* wire) {
    try {
        const std::lock_guard testLock(testSurfaceGate);
        const auto command = json::parse(wire);
        const auto op = command.at("op").get<std::string>();
        if (op == "trackAllocations" || op == "failInit") {
            if (windowLease) throw std::runtime_error("Test setup requires no live rendered surface");
            if (op == "trackAllocations" && !trackingAllocations) {
                ImGui::SetAllocatorFunctions(trackedAllocate, trackedFree);
                trackingAllocations = true;
            } else if (op == "failInit") {
                const int stage = command.at("stage").get<int>();
                if (stage < 1 || stage > 3) throw std::invalid_argument("Invalid initialization stage");
                nextInitFailure = stage;
            } else throw std::invalid_argument("Unknown test setup");
        } else {
            if (!surface) surface = testSurface;
            if (!surface || !surface->window) throw std::invalid_argument("Rendered surface required");
            auto& state = *surface->window;
            if (op == "emitEvents") {
                const int count = command.at("count").get<int>();
                if (count < 0 || count > 8192) throw std::invalid_argument("Invalid test event count");
                for (int i = 0; i < count; ++i) state.Event("click", command.value("nodeId", 1));
            } else {
                const std::lock_guard lock(state.gate);
                if (op == "failRender") state.failRender = true;
                else if (op == "failDestroy") state.failDestroy = true;
                else throw std::invalid_argument("Unknown test command");
                if (state.wakeEnabled) glfwPostEmptyEvent();
            }
        }
        return 1;
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Test injection failed"; }
    return 0;
}
extern "C" XF_API int64_t xf_test_gui_allocations() { return guiAllocations.load(); }
#endif
