#include "v8-local-handle.h"
#include "v8-maybe.h"
#include "v8-object.h"
#include "v8-primitive.h"
#include "v8-value.h"
#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

using namespace v8;
class MyObject : public node::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports);

private:
  explicit MyObject(double value = 0);
  ~MyObject();

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void PlusOne(const v8::FunctionCallbackInfo<v8::Value> &args);
  static void GetValue(v8::Local<v8::String> property,
                       const v8::PropertyCallbackInfo<v8::Value> &info);

  double value_;
};

// 初始化静态方法
void MyObject::Init(Local<Object> exports) {
  Isolate *isolate = exports->GetIsolate();

  // 准备构造函数模板
  Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
  tpl->SetClassName(String::NewFromUtf8Literal(isolate, "MyObject"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1); // 存储 C++ 对象

  // 原型方法绑定
  NODE_SET_PROTOTYPE_METHOD(tpl, "plusOne", PlusOne);

  // 实例访问器绑定
  tpl->InstanceTemplate()->SetAccessor(
      String::NewFromUtf8Literal(isolate, "value"), GetValue);

  // 导出构造函数
  Local<Function> constructor =
      tpl->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
  exports
      ->Set(isolate->GetCurrentContext(),
            String::NewFromUtf8Literal(isolate, "MyObject"), constructor)
      .ToChecked();
}

// 构造函数实现
MyObject::MyObject(double value) : value_(value) {}
MyObject::~MyObject() {}

// JavaScript 的 new MyObject() 调用
void MyObject::New(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  if (args.IsConstructCall()) {
    // 通过 new 调用
    double value =
        args[0]->IsUndefined()
            ? 0
            : args[0]->NumberValue(isolate->GetCurrentContext()).ToChecked();
    MyObject *obj = new MyObject(value);
    obj->Wrap(args.This()); // 关键：绑定 C++ 对象到 JS 实例
    Local<Function> initMethod =
        args.This()
            ->Get(args.GetIsolate()->GetCurrentContext(),
                  String::NewFromUtf8Literal(args.GetIsolate(), "_init"))
            .ToLocalChecked()
            .As<Function>();
    initMethod->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 0, {})
        .ToLocalChecked();
    args.GetReturnValue().Set(args.This());
  } else {
    args.GetIsolate()->ThrowException(
        String::NewFromUtf8Literal(args.GetIsolate(), "必须new调用"));
  }
}

// 实例方法：value += 1
void MyObject::PlusOne(const FunctionCallbackInfo<Value> &args) {
  MyObject *obj = ObjectWrap::Unwrap<MyObject>(args.Holder());
  obj->value_ += 1;
  Local<Function> emitMethod =
      args.This()
          ->Get(args.GetIsolate()->GetCurrentContext(),
                String::NewFromUtf8Literal(args.GetIsolate(), "emit"))
          .ToLocalChecked()
          .As<Function>();

  Local<Value> params[] = {
      String::NewFromUtf8Literal(args.GetIsolate(), "valuechanged"),
      Number::New(args.GetIsolate(), obj->value_),
  };
  emitMethod
      ->Call(args.GetIsolate()->GetCurrentContext(), args.Holder(), 2, params)
      .ToLocalChecked();
  args.GetReturnValue().Set(obj->value_);
}

// 属性访问器
void MyObject::GetValue(Local<String> property,
                        const PropertyCallbackInfo<Value> &info) {
  MyObject *obj = ObjectWrap::Unwrap<MyObject>(info.Holder());
  info.GetReturnValue().Set(obj->value_);
}

NODE_MODULE_INIT() {
  MyObject::Init(exports); // 初始化绑定
}