#include "v8-context.h"
#include "v8-isolate.h"
#include "v8-local-handle.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"
#include <node.h>
#include <v8.h>

v8::Local<v8::Number> DoPlusOne(v8::Local<v8::Number> ss) {
  v8::EscapableHandleScope scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Number> num =
      v8::Number::New(v8::Isolate::GetCurrent(), ss->Value() + 1);
  return scope.Escape(num);
}

v8::Local<v8::Number> DoPlusOneCrash(v8::Local<v8::Number> ss) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Number> num =
      v8::Number::New(v8::Isolate::GetCurrent(), ss->Value() + 1);
  return num;
}

void PlusOne(const v8::FunctionCallbackInfo<v8::Value> &args) {
  if (args.Length() < 1 || !args[0]->IsNumber()) {
    args.GetIsolate()->ThrowError(
        v8::String::NewFromUtf8Literal(args.GetIsolate(), "参数错误"));
    return;
  }
  v8::Local<v8::Number> result = DoPlusOne(args[0].As<v8::Number>());
  v8::Local<v8::Context> ctx = args.GetIsolate()->GetCurrentContext();
  args.GetReturnValue().Set(result);
}
void Initialize(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "plusOne", PlusOne);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)