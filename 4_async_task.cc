#include <chrono>
#include <mach/boolean.h>
#include <mutex>
#include <node.h>
#include <node_object_wrap.h>
#include <uv.h>
#include <v8.h>

using namespace v8;

typedef struct async_task_ctx_t {
  void *pointer;
  uv_async_t async;
  uv_thread_t thread;
  std::mutex mutex;
  int result;
  bool done;
} async_task_ctx_t;

class MyObject : public node::ObjectWrap {
public:
  static void Init(v8::Local<v8::Object> exports) {
    Isolate *isolate = exports->GetIsolate();

    // 准备构造函数模板
    Local<FunctionTemplate> tpl = FunctionTemplate::New(isolate, New);
    tpl->SetClassName(String::NewFromUtf8Literal(isolate, "MyObject"));
    tpl->InstanceTemplate()->SetInternalFieldCount(1); // 存储 C++ 对象

    // 原型方法绑定
    NODE_SET_PROTOTYPE_METHOD(tpl, "longTaskSync", LongTaskSync);
    NODE_SET_PROTOTYPE_METHOD(tpl, "longTaskAsync", AsyncLongTask);

    // 导出构造函数
    Local<Function> constructor =
        tpl->GetFunction(isolate->GetCurrentContext()).ToLocalChecked();
    exports
        ->Set(isolate->GetCurrentContext(),
              String::NewFromUtf8Literal(isolate, "MyObject"), constructor)
        .ToChecked();
  }

private:
  explicit MyObject(double value = 0) {}
  ~MyObject() {}

  static void New(const v8::FunctionCallbackInfo<v8::Value> &args) {
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
      initMethod
          ->Call(args.GetIsolate()->GetCurrentContext(), args.This(), 0, {})
          .ToLocalChecked();
      args.GetReturnValue().Set(args.This());
    } else {
      args.GetIsolate()->ThrowException(
          String::NewFromUtf8Literal(args.GetIsolate(), "必须new调用"));
    }
  }

  static void LongTaskSync(const v8::FunctionCallbackInfo<v8::Value> &args) {
    MyObject *obj = ObjectWrap::Unwrap<MyObject>(args.Holder());
    int longTask = obj->DoLongTask(obj);
    args.GetReturnValue().Set(longTask);
  }

  static void LongTaskChildThread(void *ctx) {
    // 这里在子线程
    async_task_ctx_t *asyc_task_ctx = (async_task_ctx_t *)ctx;
    int result = DoLongTask((MyObject *)asyc_task_ctx->pointer);
    std::lock_guard<std::mutex> guard(asyc_task_ctx->mutex);
    asyc_task_ctx->result = result;
    asyc_task_ctx->done = true;
    uv_async_send(&asyc_task_ctx->async);
  }

  static void LongTaskUVAsynCallback(uv_async_t *async) {
    // 这里在主线程
    async_task_ctx_t *asyc_task_ctx = (async_task_ctx_t *)async->data;
    bool done = false;
    int result = 0;
    {
      std::lock_guard<std::mutex> guard(asyc_task_ctx->mutex);
      done = asyc_task_ctx->done;
      result = asyc_task_ctx->result;
    }
    if (done) {
      uv_thread_join(&asyc_task_ctx->thread);
      uv_close((uv_handle_t *)&asyc_task_ctx->async, NULL);
      HandleScope scope(v8::Isolate::GetCurrent());
      MyObject *obj = (MyObject *)asyc_task_ctx->pointer;
      Local<Value> params[] = {
          String::NewFromUtf8Literal(v8::Isolate::GetCurrent(), "result"),
          Number::New(Isolate::GetCurrent(), result)};
      Local<Function> emitMethod =
          obj->handle()
              ->Get(
                  v8::Isolate::GetCurrent()->GetCurrentContext(),
                  String::NewFromUtf8Literal(v8::Isolate::GetCurrent(), "emit"))
              .ToLocalChecked()
              .As<Function>();
      emitMethod
          ->Call(v8::Isolate::GetCurrent()->GetCurrentContext(), obj->handle(),
                 2, params)
          .ToLocalChecked();
      obj->Unref();
      delete asyc_task_ctx;
    }
  }

  static int DoLongTask(MyObject *obj) {
    uv_sleep(2000);
    std::chrono::time_point<std::chrono::system_clock> now =
        std::chrono::system_clock::now();

    int timestamp_s =
        std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch())
            .count();
    return timestamp_s;
  }

  static void AsyncLongTask(const v8::FunctionCallbackInfo<v8::Value> &args) {
    node::ObjectWrap::Unwrap<MyObject>(args.Holder())->Ref();
    async_task_ctx_t *ctx = new async_task_ctx_t();
    ctx->pointer = node::ObjectWrap::Unwrap<MyObject>(args.Holder());
    ctx->async.data = ctx;
    ctx->result = 0;
    ctx->done = false;
    uv_async_init(uv_default_loop(), &ctx->async, &LongTaskUVAsynCallback);
    uv_thread_create(&ctx->thread, LongTaskChildThread, ctx);
  }
};

NODE_MODULE_INIT() {
  MyObject::Init(exports); // 初始化绑定
}
