
#include <xframes-wrapper.h>
#include <xframes-runner.h>

#ifdef __cplusplus
extern "C" {
#endif

Runner* Runner::instance = nullptr;

void resizeWindow(const int width, const int height) {
    Runner* pRunner = Runner::getInstance();
    pRunner->resizeWindow(width, height);
}

void setElement(std::string& elementJson) {
    Runner* pRunner = Runner::getInstance();

    pRunner->setElement(elementJson);
}

void patchElement(int id, std::string& elementJson) {
    Runner* pRunner = Runner::getInstance();

    pRunner->patchElement(id, elementJson);
}

void elementInternalOp(int id, std::string& elementJson) {
    Runner* pRunner = Runner::getInstance();

    pRunner->elementInternalOp(id, elementJson);
}

void setChildren(int id, std::string& childrenIds) {
    Runner* pRunner = Runner::getInstance();
    // todo: use array of numbers instead of parsing JSON
    pRunner->setChildren((int)id, JsonToVector<int>(childrenIds));
}

void appendChild(int parentId, int childId) {
    auto pRunner = Runner::getInstance();
    
    pRunner->appendChild(parentId, childId);
}

std::string getChildren(int id) {
    Runner* pRunner = Runner::getInstance();
    
    return IntVectorToJson(pRunner->getChildren(id)).dump();
}

void appendTextToClippedMultiLineTextRenderer(int id, std::string& data) {
    Runner* pRunner = Runner::getInstance();

    pRunner->appendTextToClippedMultiLineTextRenderer(id, data);
}

std::string getStyle() {
    Runner* pRunner = Runner::getInstance();
    
    return pRunner->getStyle();
}

void patchStyle(std::string& styleDef) {
    Runner* pRunner = Runner::getInstance();
    
    pRunner->patchStyle(styleDef);
}

void setDebug(const bool debug) {
    Runner* pRunner = Runner::getInstance();

    pRunner->setDebug(debug);
}

void showDebugWindow() {
    Runner* pRunner = Runner::getInstance();

    pRunner->showDebugWindow();
}

int run()
{
    Runner* pRunner = Runner::getInstance();

    pRunner->run();

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
static void init(
    const char* assetsBasePath,
    const char* rawFontDefinitions,
    const char* rawStyleOverrideDefinitions,
    OnInitCb& onInit,
    OnTextChangedCb& onTextChanged,
    OnComboChangedCb& onComboChanged,
    OnNumericValueChangedCb& onNumericValueChanged,
    OnBooleanValueChangedCb& onBooleanValueChanged,
    OnMultipleNumericValuesChangedCb& onMultipleNumericValuesChanged,
    OnClickCb& onClick
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

    pRunner->init();

    // uiThread = std::thread(run);
    // uiThread.detach();
}

#ifdef __cplusplus
}
#endif

