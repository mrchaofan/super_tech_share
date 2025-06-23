#include "v8-isolate.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"
#include <node.h>
#include <v8.h>

v8::Local<v8::Value> lv;
v8::Persistent<v8::Value> pv;
v8::Persistent<v8::Function> cb;

void CallCallback(const v8::FunctionCallbackInfo<v8::Value> &args) {
  v8::Local<v8::Value> params[] = {lv, pv.Get(args.GetIsolate())};
  cb.Get(args.GetIsolate())
      ->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 2, params)
      .ToLocalChecked();
}

void SetAsLocal(const v8::FunctionCallbackInfo<v8::Value> &args) {
  if (args.Length() < 1) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "参数错误"));
    return;
  }
  lv = args[0];
  pv.Reset(args.GetIsolate(), args[0]);

  
  if (args.Length() >= 2 && args[1]->IsBoolean() && args[1].As<v8::Boolean>()->Value() == true) {
    v8::Local<v8::Value> params[] = {lv, pv.Get(args.GetIsolate())};
    cb.Get(args.GetIsolate())
        ->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 2, params)
        .ToLocalChecked();
  }
}

void SetCallback(const v8::FunctionCallbackInfo<v8::Value> &args) {
  if (args.Length() < 1 || !args[0]->IsFunction()) {
    args.GetIsolate()->ThrowException(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "参数错误"));
    return;
  }
  cb.Reset(args.GetIsolate(), args[0].As<v8::Function>());
}

void Initialize(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "setAsLocal", SetAsLocal);
  NODE_SET_METHOD(exports, "callCallback", CallCallback);
  NODE_SET_METHOD(exports, "setCallback", SetCallback);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)