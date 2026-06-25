#ifndef V8GO_OBJECT_TEMPLATE_H
#define V8GO_OBJECT_TEMPLATE_H

#include "errors.h"
#include "template.h"

#ifdef __cplusplus

namespace v8 {
class Isolate;
}

typedef v8::Isolate v8Isolate;

extern "C" {
#else

typedef struct v8Isolate v8Isolate;

#endif

typedef struct m_ctx m_ctx;
typedef struct m_template m_template;

// v8goPropertyCallbackInfo represents the properties of v8's
// PropertyCallbackInfo that are sent from V8 to the embedder, i.e., the return
// value is excluded. Because this doesn't include the property, it can be used
// for both named and indexed properties.
typedef struct {
  int ctx_ref;
  ValuePtr cbref;
  ValuePtr jsThis;
  ValuePtr holder;
} v8goPropertyCallbackInfo;

extern TemplatePtr NewObjectTemplate(v8Isolate* iso_ptr);
extern RtnValue ObjectTemplateNewInstance(m_template* ptr, m_ctx* ctx_ptr);
extern void ObjectTemplateSetInternalFieldCount(m_template* ptr,
                                                int field_count);
extern int ObjectTemplateInternalFieldCount(m_template* ptr);
extern void ObjectTemplateSetAccessorProperty(m_template* ptr,
                                              const char* key,
                                              m_template* get,
                                              m_template* set,
                                              int attributes);
extern void ObjectTemplateMarkAsUndetectable(m_template* ptr);
extern void ObjectTemplateSetCallAsFunctionHandler(m_template* ptr,
                                                   int callback_ref);
extern void ObjectTemplateSetNamedHandler(TemplatePtr ptr, ValuePtr cb_ref);
extern void ObjectTemplateSetIndexHandler(TemplatePtr ptr,
                                          int get_callback_ref);

#ifdef __cplusplus
}
#endif
#endif
