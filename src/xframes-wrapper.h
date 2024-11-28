#ifndef XFRAMES_WRAPPER_H
#define XFRAMES_WRAPPER_H

#include <stdbool.h>
#include <xframes-runner.h>

#ifdef _WIN32
#define EXPORT_API __declspec(dllexport)  // Import symbols from DLL
#else
#define EXPORT_API __attribute__((visibility("default")))  // For non-Windows
#endif

#ifdef __cplusplus
extern "C" {
#endif

EXPORT_API void resizeWindow(const int width, const int height);
EXPORT_API void setElement(const char* elementJson);
EXPORT_API void patchElement(int id, const char* elementJson);
EXPORT_API void elementInternalOp(int id, const char* elementJson);
EXPORT_API void setChildren(int id, const char* childrenIds);
EXPORT_API void appendChild(int parentId, int childId);
EXPORT_API const char* getChildren(int id);
EXPORT_API void appendTextToClippedMultiLineTextRenderer(int id, const char* data);
EXPORT_API const char* getStyle();
EXPORT_API void patchStyle(const char* styleDef);
EXPORT_API void setDebug(const bool debug);
EXPORT_API void showDebugWindow();
EXPORT_API void init(
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
);

#ifdef __cplusplus
}
#endif

#endif // XFRAMES_WRAPPER_H