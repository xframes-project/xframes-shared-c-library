#ifndef XFRAMES_TRANSACTION_H
#define XFRAMES_TRANSACTION_H

#if defined(_WIN32)
#if defined(XFRAMES_SHARED_BUILD)
#define XF_API __declspec(dllexport)
#else
#define XF_API __declspec(dllimport)
#endif
#else
#define XF_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct xf_surface xf_surface;
/* All strings are UTF-8. Results are borrowed until the next call on this handle.
 * Callers serialize handle access and must copy strings before another call.
 * Null results indicate an unknown native outcome: inspect xf_last_error, then quarantine.
 * No C++ exceptions cross this ABI. Headless creation runs the production commit path. */
XF_API unsigned xf_abi_version(void);
XF_API const char* xf_last_error(void);
XF_API xf_surface* xf_create(void);
XF_API const char* xf_apply(xf_surface* surface, const char* wire);
XF_API const char* xf_state(xf_surface* surface);
XF_API const char* xf_diagnostics(xf_surface* surface);
/* Success proves safe release. On failure retain the handle and dependent resources. */
XF_API int xf_destroy(xf_surface* surface);

#ifdef __cplusplus
}
#endif
#endif
