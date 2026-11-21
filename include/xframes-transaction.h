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
/* Additive ABI-1 Windows extension. At most one rendered surface per process.
 * options: {title,width,height,visible,diagnostics}; creation waits for initialization.
 * The renderer owns its Windows thread. Events are queued, never reentrant into callers. */
XF_API xf_surface* xf_create_window(const char* options);
XF_API const char* xf_poll_events(xf_surface* surface);
XF_API const char* xf_frame(xf_surface* surface);
/* Commands: {op:"close"}, {op:"resize",width,height}, {op:"screenshot",path}.
 * Screenshot completion/error arrives in the event queue. */
XF_API int xf_window_command(xf_surface* surface, const char* command);
XF_API const char* xf_apply(xf_surface* surface, const char* wire);
/* Additive ABI-1 extension: forward one widget operation, such as a table's or plot's
 * {op:"setData",...} or {op:"appendData",...}, to a live node created by xf_apply.
 * operation is a JSON object with a string "op". Returns 1 once the widget has handled it,
 * serialized with commits. Returns 0 and sets xf_last_error for malformed JSON, a missing op,
 * an unknown node or a quarantined surface. Operation names a widget does not know are ignored. */
XF_API int xf_element_op(xf_surface* surface, int id, const char* operation);
XF_API const char* xf_state(xf_surface* surface);
XF_API const char* xf_diagnostics(xf_surface* surface);
/* Success proves safe release. On failure retain the handle and dependent resources. */
XF_API int xf_destroy(xf_surface* surface);

#ifdef __cplusplus
}
#endif
#endif
