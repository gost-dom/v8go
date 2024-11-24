#include "_cgo_export.h"

#include "context.h"
#include "deps/include/v8-context.h"
#include "deps/include/v8-isolate.h"
#include "deps/include/v8-locker.h"
#include "deps/include/v8-template.h"
#include "isolate-macros.h"
#include "object_template.h"
#include "template-macros.h"

using namespace v8;

static Intercepted PropertyCallback(uint32_t index,
                                    const PropertyCallbackInfo<Value>& info) {
  Isolate* iso = info.GetIsolate();
  ISOLATE_SCOPE(iso);

  // This callback function can be called from any Context, which we only know
  // at runtime. We extract the Context reference from the embedder data so that
  // we can use the context registry to match the Context on the Go side
  Local<Context> local_ctx = iso->GetCurrentContext();
  int ctx_ref = local_ctx->GetEmbedderData(1).As<Integer>()->Value();
  m_ctx* ctx = goContext(ctx_ref);

  int callback_ref = info.Data().As<Integer>()->Value();

  m_value* _this = new m_value;
  _this->id = 0;
  _this->iso = iso;
  _this->ctx = ctx;
  _this->ptr.Reset(iso, Global<Value>(iso, info.This()));

  // int args_count = info.Length();
  ValuePtr thisAndArgs[1];
  thisAndArgs[0] = tracked_value(ctx, _this);
  // ValuePtr *args = thisAndArgs + 1;
  // for (int i = 0; i < args_count; i++) {
  //   m_value *val = new m_value;
  //   val->id = 0;
  //   val->iso = iso;
  //   val->ctx = ctx;
  //   val->ptr.Reset(iso, Global<Value>(iso, info[i]));
  //   args[i] = tracked_value(ctx, val);
  // }

  goFunctionCallback_return retval =
      goFunctionCallback(ctx_ref, callback_ref, thisAndArgs, 0, index);
  if (retval.r1 != nullptr) {
    iso->ThrowException(retval.r1->ptr.Get(iso));
  } else if (retval.r0 != nullptr) {
    info.GetReturnValue().Set(retval.r0->ptr.Get(iso));
  } else {
    info.GetReturnValue().SetUndefined();
  }
  return v8::Intercepted::kYes;
}

TemplatePtr NewObjectTemplate(v8Isolate* iso) {
  Locker locker(iso);
  Isolate::Scope isolate_scope(iso);
  HandleScope handle_scope(iso);

  m_template* ot = new m_template;
  ot->iso = iso;
  ot->ptr.Reset(iso, ObjectTemplate::New(iso));
  return ot;
}

RtnValue ObjectTemplateNewInstance(TemplatePtr ptr, m_ctx* ctx) {
  LOCAL_TEMPLATE(ptr);
  TryCatch try_catch(iso);
  Local<Context> local_ctx = ctx->ptr.Get(iso);
  Context::Scope context_scope(local_ctx);

  RtnValue rtn = {};

  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  Local<Object> obj;
  if (!obj_tmpl->NewInstance(local_ctx).ToLocal(&obj)) {
    rtn.error = ExceptionError(try_catch, iso, local_ctx);
    return rtn;
  }

  m_value* val = new m_value;
  val->id = 0;
  val->iso = iso;
  val->ctx = ctx;
  val->ptr = Global<Value>(iso, obj);
  rtn.value = tracked_value(ctx, val);
  return rtn;
}

void ObjectTemplateSetInternalFieldCount(TemplatePtr ptr, int field_count) {
  LOCAL_TEMPLATE(ptr);

  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->SetInternalFieldCount(field_count);
}

int ObjectTemplateInternalFieldCount(TemplatePtr ptr) {
  LOCAL_TEMPLATE(ptr);

  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  return obj_tmpl->InternalFieldCount();
}

void ObjectTemplateSetAccessorProperty(TemplatePtr ptr,
                                       const char* key,
                                       TemplatePtr get,
                                       TemplatePtr set,
                                       int attributes) {
  LOCAL_TEMPLATE(ptr);

  /*
   *
  Isolate* iso = tmpl_ptr->iso;      \
  Locker locker(iso);                \
  Isolate::Scope isolate_scope(iso); \
  HandleScope handle_scope(iso);     \
  Local<Template> tmpl = tmpl_ptr->ptr.Get(iso);
   */
  Local<String> key_val =
      String::NewFromUtf8(iso, key, NewStringType::kNormal).ToLocalChecked();
  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  Local<FunctionTemplate> get_tmpl =
      get ? get->ptr.Get(iso).As<FunctionTemplate>()
          : Local<FunctionTemplate>();
  Local<FunctionTemplate> set_tmpl =
      set ? set->ptr.Get(iso).As<FunctionTemplate>()
          : Local<FunctionTemplate>();

  return obj_tmpl->SetAccessorProperty(key_val, get_tmpl, set_tmpl,
                                       (PropertyAttribute)attributes);
}

void ObjectTemplateSetIndexHandler(TemplatePtr ptr, int get_callback_ref) {
  LOCAL_TEMPLATE(ptr);
  Local<Integer> cbData = Integer::New(iso, get_callback_ref);
  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->SetHandler(IndexedPropertyHandlerConfiguration(
      PropertyCallback, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      cbData, PropertyHandlerFlags::kHasNoSideEffect));
}
