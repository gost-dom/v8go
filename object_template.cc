#include "_cgo_export.h"

#include "context.h"
#include "deps/include/v8-context.h"
#include "deps/include/v8-isolate.h"
#include "deps/include/v8-locker.h"
#include "deps/include/v8-template.h"
#include "function_template.h"
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
  int ctx_ref = local_ctx->GetEmbedderDataV2(1).As<Integer>()->Value();
  m_ctx* ctx = goContext(ctx_ref);

  int callback_ref = info.Data().As<Integer>()->Value();

  m_value* _this = new m_value;
  _this->id = 0;
  _this->iso = iso;
  _this->ctx = ctx;
  // V8 removed PropertyCallbackInfo::This(); the receiver accessor for
  // interceptors is now Holder(). For interceptors installed directly on the
  // object (as v8go does), the receiver and holder are the same object.
  _this->ptr.Reset(iso, Global<Value>(iso, info.Holder()));

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

template <class T>
v8goPropertyCallbackInfo convertCallback(
    v8::Isolate* iso,
    const v8::PropertyCallbackInfo<T>& info,
    m_ctx*& ctx) {
  Local<Context> local_ctx = iso->GetCurrentContext();

  int ctx_ref = local_ctx->GetEmbedderDataV2(1).As<Integer>()->Value();
  ctx = goContext(ctx_ref);

  v8goPropertyCallbackInfo rtnVal;
  rtnVal.ctx_ref = ctx_ref;
  rtnVal.cbref = track_value(ctx, info.Data());
  // PropertyCallbackInfo::This() was removed; Holder() is the available
  // receiver accessor now (HolderV2() is deprecated in favor of Holder()).
  rtnVal.jsThis = track_value(ctx, info.Holder());
  rtnVal.holder = track_value(ctx, info.Holder());
  return rtnVal;
}

template <class T>
Intercepted HandleRtnVal(m_value* value,
                         bool intercepted,
                         m_value* errVal,
                         const v8::PropertyCallbackInfo<T>& info) {
  Isolate* iso = info.GetIsolate();
  if (errVal != nullptr) {
    iso->ThrowException(errVal->ToLocal());
  }
  if (value != nullptr) {
    info.GetReturnValue().Set(value->ToLocal().As<T>());
  }
  if (intercepted) {
    return Intercepted::kYes;
  } else {
    return Intercepted::kNo;
  }
}

Intercepted HandleVoidRtnVal(bool intercepted,
                             m_value* errVal,
                             const v8::PropertyCallbackInfo<void>& info) {
  Isolate* iso = info.GetIsolate();
  if (errVal != nullptr) {
    iso->ThrowException(errVal->ToLocal());
  }
  if (intercepted) {
    return Intercepted::kYes;
  } else {
    return Intercepted::kNo;
  }
}

Intercepted HandleBoolRtnVal(bool success,
                             bool intercepted,
                             m_value* errVal,
                             const v8::PropertyCallbackInfo<Boolean>& info) {
  Isolate* iso = info.GetIsolate();
  if (errVal != nullptr) {
    iso->ThrowException(errVal->ToLocal());
  }
  info.GetReturnValue().Set(v8::Boolean::New(iso, success));
  if (intercepted) {
    return Intercepted::kYes;
  } else {
    return Intercepted::kNo;
  }
}

Intercepted GetterCallback(Local<Name> name,
                           const v8::PropertyCallbackInfo<v8::Value>& info) {
  Isolate* iso = info.GetIsolate();
  ISOLATE_SCOPE(iso);

  m_ctx* ctx;
  v8goPropertyCallbackInfo goInfo = convertCallback(iso, info, ctx);

  goNamedPropertyGetterCallback_return retval =
      goNamedPropertyGetterCallback(track_value(ctx, name), goInfo);

  return HandleRtnVal<Value>(retval.r0, retval.r1, retval.r2, info);
}

// NamedPropertySetterCallbackV2: the setter now reports success via a
// PropertyCallbackInfo<Boolean> return value (false makes a strict-mode
// assignment throw). We report success whenever the Go side intercepted.
Intercepted SetterCallback(Local<Name> name,
                           Local<Value> value,
                           const v8::PropertyCallbackInfo<v8::Boolean>& info) {
  Isolate* iso = info.GetIsolate();
  ISOLATE_SCOPE(iso);

  m_ctx* ctx;
  v8goPropertyCallbackInfo goInfo = convertCallback(iso, info, ctx);

  goNamedPropertySetterCallback_return retval = goNamedPropertySetterCallback(
      track_value(ctx, name), track_value(ctx, value), goInfo);

  return HandleBoolRtnVal(retval.r0, retval.r0, retval.r1, info);
}

Intercepted DeleterCallback(Local<Name> name,
                            const v8::PropertyCallbackInfo<Boolean>& info) {
  Isolate* iso = info.GetIsolate();
  ISOLATE_SCOPE(iso);

  m_ctx* ctx;
  v8goPropertyCallbackInfo goInfo = convertCallback(iso, info, ctx);

  goNamedPropertyDeleterCallback_return retval =
      goNamedPropertyDeleterCallback(track_value(ctx, name), goInfo);

  return HandleBoolRtnVal(retval.r0, retval.r1, retval.r2, info);
}

void EnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>& info) {
  Isolate* iso = info.GetIsolate();
  ISOLATE_SCOPE(iso);

  m_ctx* ctx;
  v8goPropertyCallbackInfo goInfo = convertCallback(iso, info, ctx);

  goNamedPropertyEnumeratorCallback_return retval =
      goNamedPropertyEnumeratorCallback(goInfo);

  HandleRtnVal<v8::Array>(retval.r0, retval.r1, retval.r2, info);
}

void ObjectTemplateSetNamedHandler(TemplatePtr ptr, ValuePtr callback_ref) {
  LOCAL_TEMPLATE(ptr);
  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->SetHandler(NamedPropertyHandlerConfiguration(
      GetterCallback, SetterCallback, /* query */ nullptr, DeleterCallback,
      EnumeratorCallback, nullptr, nullptr, callback_ref->ToLocal(),
      PropertyHandlerFlags::kHasNoSideEffect));
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

void ObjectTemplateMarkAsUndetectable(TemplatePtr ptr) {
  LOCAL_TEMPLATE(ptr);

  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->MarkAsUndetectable();
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

void ObjectTemplateSetCallAsFunctionHandler(TemplatePtr ptr, int callback_ref) {
  LOCAL_TEMPLATE(ptr);

  Local<Integer> cbData = Integer::New(iso, callback_ref);

  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->SetCallAsFunctionHandler(FunctionTemplateCallback, cbData);
}

void ObjectTemplateSetIndexHandler(TemplatePtr ptr, int get_callback_ref) {
  LOCAL_TEMPLATE(ptr);
  Local<Integer> cbData = Integer::New(iso, get_callback_ref);
  Local<ObjectTemplate> obj_tmpl = tmpl.As<ObjectTemplate>();
  obj_tmpl->SetHandler(IndexedPropertyHandlerConfiguration(
      PropertyCallback, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
      cbData, PropertyHandlerFlags::kHasNoSideEffect));
}
