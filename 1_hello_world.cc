#include <node.h>
#include <v8.h>
#include <uv.h>

void Method(const v8::FunctionCallbackInfo<v8::Value>& args) {
  uv_sleep(1000);
  args.GetReturnValue().Set(v8::String::NewFromUtf8(
    args.GetIsolate(), "Hello World").ToLocalChecked());
}
void Initialize(v8::Local<v8::Object> exports) {
  NODE_SET_METHOD(exports, "hello", Method);
}
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)