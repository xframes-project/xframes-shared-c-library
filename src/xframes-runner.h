#ifndef XFRAMES_H
#define XFRAMES_H

#include <thread>
#include <cstdio>
#include <utility>
#include <vector>
#include <optional>
#include <functional>
#include <string>
#include <set>

#include <GLFW/glfw3.h>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "implot.h"
#include "implot_internal.h"
#include <nlohmann/json.hpp>

#include "color_helpers.h"
#include "xframes.h"
#include "implot_renderer.h"

#include <nlohmann/json.hpp>

using OnInitCb = const std::function<void()>;
using OnTextChangedCb = const std::function<void(int, const std::string&)>;
using OnComboChangedCb = const std::function<void(int, int)>;
using OnNumericValueChangedCb = const std::function<void(int, float)>;
using OnBooleanValueChangedCb = const std::function<void(int, bool)>;
using OnMultipleNumericValuesChangedCb = const std::function<void(int, std::vector<float>)>;
using OnClickCb = const std::function<void(int)>;

using json = nlohmann::json;

template <typename T>
std::vector<T> JsonToVector(std::string& data);

template <typename T>
std::set<T> JsonToSet(std::string& data);

json IntVectorToJson(const std::vector<int>& data);
json IntSetToJson(const std::set<int>& data);

class Runner {
    protected:
        XFrames* m_xframes{};
        ImGuiRenderer* m_renderer{};

        std::string m_rawFontDefs;
        std::string m_assetsBasePath;
        std::optional<std::string> m_rawStyleOverridesDefs;

        std::function<void()> m_onInit;
        std::function<void(int, const std::string&)> m_onTextChange;
        std::function<void(int, int)> m_onComboChange;
        std::function<void(int, float)> m_onNumericValueChange;
        std::function<void(int, bool)> m_onBooleanValueChange;
        std::function<void(int, std::vector<float>)> m_onMultipleNumericValuesChange;
        std::function<void(int)> m_onClick;

        static Runner* instance;

        Runner();
    public:
        static Runner* getInstance();

        ~Runner();

        static void OnInit();

        static void OnTextChange(const int id, const std::string& value);

        static void OnComboChange(const int id, const int value);

        static void OnNumericValueChange(const int id, const float value);

        static void OnBooleanValueChange(const int id, const bool value);

        // todo: improve
        static void OnMultipleNumericValuesChange(const int id, const float* values, const int numValues);

        static void OnClick(int id);

        void SetHandlers(
            OnInitCb& onInit,
            OnTextChangedCb& onTextChanged,
            OnComboChangedCb& onComboChanged,
            OnNumericValueChangedCb& onNumericValueChanged,
            OnBooleanValueChangedCb& onBooleanValueChanged,
            OnMultipleNumericValuesChangedCb& onMultipleNumericValuesChanged,
            OnClickCb& onClick
            );

        void SetRawFontDefs(std::string rawFontDefs);

        void SetAssetsBasePath(std::string basePath);

        void SetRawStyleOverridesDefs(const std::string& rawStyleOverridesDefs);

        void init();

        void run();

        void exit();

        void resizeWindow(const int width, const int height);

        void setElement(std::string& elementJsonAsString);

        void patchElement(const int id, std::string& elementJsonAsString);

        void elementInternalOp(const int id, std::string& elementJsonAsString);

        void setChildren(const int id, const std::vector<int>& childrenIds);

        void appendChild(const int parentId, const int childId);

        std::vector<int> getChildren(const int id);

        std::string getAvailableFonts();

        void appendTextToClippedMultiLineTextRenderer(const int id, const std::string& data);

        std::string getStyle();

        void patchStyle(std::string& styleDef);

        void setDebug(const bool debug);

        void showDebugWindow();
};

#endif // XFRAMES_H