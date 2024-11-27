
#include <xframes-wrapper.h>
#include <xframes-runner.h>

Runner* Runner::instance = nullptr;

#ifdef __cplusplus
extern "C" {
#endif

void resizeWindow(const int width, const int height) {
    Runner* pRunner = Runner::getInstance();
    pRunner->ResizeWindow(width, height);
}

void setElement(const char* elementJson) {
    Runner* pRunner = Runner::getInstance();
    std::string elementJsonString = elementJson;

    pRunner->SetElement(elementJsonString);
}

void patchElement(int id, const char* elementJson) {
    Runner* pRunner = Runner::getInstance();
    std::string elementJsonString = elementJson;

    pRunner->PatchElement(id, elementJsonString);
}

void elementInternalOp(int id, const char* elementJson) {
    Runner* pRunner = Runner::getInstance();
    std::string elementJsonString = elementJson;

    pRunner->ElementInternalOp(id, elementJsonString);
}

void setChildren(int id, const char* childrenIds) {
    Runner* pRunner = Runner::getInstance();
    std::string childrenIdsString = childrenIds;
    // todo: use array of numbers instead of parsing JSON
    pRunner->SetChildren((int)id, JsonToVector<int>(childrenIdsString));
}

void appendChild(int parentId, int childId) {
    Runner* pRunner = Runner::getInstance();
    
    pRunner->AppendChild(parentId, childId);
}

const char* getChildren(int id) {
    Runner* pRunner = Runner::getInstance();
    
    return IntVectorToJson(pRunner->GetChildren(id)).dump().c_str();
}

void appendTextToClippedMultiLineTextRenderer(int id, const char* data) {
    Runner* pRunner = Runner::getInstance();
    std::string dataString = data;

    pRunner->AppendTextToClippedMultiLineTextRenderer(id, dataString);
}

const char* getStyle() {
    Runner* pRunner = Runner::getInstance();
    
    return pRunner->GetStyle().c_str();
}

void patchStyle(const char* styleDef) {
    Runner* pRunner = Runner::getInstance();
    std::string styleDefString = styleDef;
    
    pRunner->PatchStyle(styleDefString);
}

void setDebug(const bool debug) {
    Runner* pRunner = Runner::getInstance();

    pRunner->SetDebug(debug);
}

void showDebugWindow() {
    Runner* pRunner = Runner::getInstance();

    pRunner->ShowDebugWindow();
}

int run()
{
    Runner* pRunner = Runner::getInstance();

    pRunner->Run();

    return 0;
}

// std::thread uiThread;


// std::thread nativeThread;

/**
 * [0] assets base path
 * [1] raw font definitions (stringified JSON)
 * [2] raw style override definitions (stringified JSON)
 * [3] onInit function
 * [4] onTextChanged function
 * [5] onComboChanged function
 * [6] onNumericValueChanged function
 * [7] OnBooleanValueChanged function
 * [8] OnMultipleNumericValuesChanged function
 * [9] OnClick function
 */
void init(
    const char* assetsBasePath,
    const char* rawFontDefinitions,
    const char* rawStyleOverrideDefinitions,
    OnInitCb onInit,
    OnTextChangedCb onTextChanged,
    OnComboChangedCb onComboChanged,
    OnNumericValueChangedCb onNumericValueChanged,
    OnBooleanValueChangedCb onBooleanValueChanged,
    OnMultipleNumericValuesChangedCb onMultipleNumericValuesChanged,
    OnClickCb onClick
    ) {
    Runner* pRunner = Runner::getInstance();

    pRunner->SetAssetsBasePath(assetsBasePath);
    pRunner->SetRawFontDefs(rawFontDefinitions);
    pRunner->SetRawStyleOverridesDefs(rawStyleOverrideDefinitions);

    pRunner->SetHandlers(
        onInit,
        onTextChanged,
        onComboChanged,
        onNumericValueChanged,
        onBooleanValueChanged,
        onMultipleNumericValuesChanged,
        onClick
    );

    pRunner->Init();

    // uiThread = std::thread(run);
    // uiThread.detach();
}

#ifdef __cplusplus
}
#endif

