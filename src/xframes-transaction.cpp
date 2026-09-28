#include <xframes-transaction.h>
#include <memory>
#include <string>
#include <xframes.h>

struct xf_surface {
    XFrames runtime{"xframes-shared", std::nullopt};
    std::string result;
};

namespace {
thread_local std::string lastError;
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

unsigned xf_abi_version() { return 1; }
const char* xf_last_error() { return lastError.c_str(); }
xf_surface* xf_create() {
    try { return new xf_surface(); }
    catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Native initialization failed"; }
    return nullptr;
}
const char* xf_apply(xf_surface* surface, const char* wire) {
    if (!wire) { lastError = "Null commit payload"; return nullptr; }
    return jsonCall(surface, [wire](XFrames& runtime) { return runtime.ApplyCommit(wire).ToJson(); });
}
const char* xf_state(xf_surface* surface) {
    return jsonCall(surface, [](XFrames& runtime) { return runtime.GetCommitState(); });
}
const char* xf_diagnostics(xf_surface* surface) {
    return jsonCall(surface, [](XFrames& runtime) { return runtime.GetDiagnosticsState(); });
}
int xf_destroy(xf_surface* surface) {
    try {
        if (surface) surface->runtime.Dispose();
        delete surface;
        return 1;
    } catch (const std::exception& error) { lastError = error.what(); }
    catch (...) { lastError = "Native teardown failed"; }
    return 0;
}
