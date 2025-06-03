# 刀耕火种的方式开发Node Addon - V8 & libuv

## 预备知识 & 环境准备

### 动态库

node addon的本质就是动态库，只是扩展名是.node。程序可以调用动态库的方法([main.cc](动态库中调用程序的方法/main.cc)),动态库也可以调用程序提供的方法。([b.cc](动态库中调用程序的方法/b.cc))

### 环境准备

编译node addon只需要头文件，无需链接任何库。
从[Github](https://github.com/electron/electron/releases)下载头文件(.h)。

### 编译

[build.js](build.js)

编译没有使用黑盒一般的gyp，使用单行透明命令～

```bash
clang++ -g -shared -fPIC -std=c++20 -undefined dynamic_lookup -I./include $源码名称.cc -o $输出名称.node
```

## 出发

### [Hello World](1_hello_world.cc)

```c++
#include <node.h>
NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize);
```

注意NODE_MODULE是宏不是函数，展开后最终会调用node_module_register方法，执行模块的Initialize方法，允许开发导出自己的函数。开发node addon时，直接在动态库中调用程序中的node方法与node交互。

感兴趣进一步阅读

[loader.js](https://github.com/nodejs/node/blob/main/lib/internal/modules/cjs/loader.js)

[node_binding.cc](https://github.com/nodejs/node/blob/main/src/node_binding.cc)

### [V8对象的生命周期](2_v8_object_lifecycle_0.cc)

一个简单的v8函数，返回一个数字。

```c++
v8::Local<v8::Number> DoPlusOne(v8::Local<v8::Number> ss) {
  v8::HandleScope scope(v8::Isolate::GetCurrent());
  v8::Local<v8::Number> num =
      v8::Number::New(v8::Isolate::GetCurrent(), ss->Value() + 1);
  return num;
}
```

调用后会崩溃。在V8引擎中不能直接获取v8::Value（JavaScript对象），必须通过句柄操作JavaScript对象。其中v8::Local是最简单最常用的句柄。v8::Local可以简单理解为智能指针std::shared_ptr，v8::HandleScope可以理解为智能指针的容器std::vector&lt;std::shared_ptr&gt;。当vector析构时，所有的shared_ptr也会析构，从而释放对应的堆分配内存。

DoPlusOne函数返回了Local变量num，当scope销毁时，Local和local对应的JavaScript对象都会被销毁。v8提供了一些对象内存管理的，针对这个例子可以用v8::EscapableHandleScope避免num对象析构。

除了v8::Local外还有v8::Persist。v8::Persist的生命周期不受函数调用栈的限制，一般会当作堆分配对象的属性，在对象析构时销毁。

### 封装C++ class为JavaScript的class

#### 建立JavaScript this和C++对象的双向绑定关系

C++的class可以继承node::ObjectWrap类，在JavaScript执行构造函数时，

```javascript
new MyObject();
```

会调用实例化函数模版是设置的初始化函数，它是一个静态方法。

```c++
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
    args.GetReturnValue().Set(args.This());
  } else {
    args.GetIsolate()->ThrowException(
        String::NewFromUtf8Literal(args.GetIsolate(), "必须new调用"));
  }
}
```

关键是使用Wrap方法，将JavaScript的this与C++对象建立双向绑定关系。通过JavaScript的this获取到C++的对象，通过C++的对象获取JavaScript的this。

#### 通过JavaScript获取C++对象

在JavaScript调用对象方法时，

```javascript
myobject.plusOne();
```

会调用c++的静态方法，

```c++
void MyObject::PlusOne(const FunctionCallbackInfo<Value> &args) {
  MyObject *obj = ObjectWrap::Unwrap<MyObject>(args.Holder());  
}
```

可以通过args.Holder获取的this，通过绑定关系获取到C++的对象。（不要纠结this和holder的区别，类似于Reflect里面的receiver，在addon开发实践中，可以背下来构造函数用args.This，原型方法用args.Holder🐶）。

#### 通过C++对象获取JavaScript对象

```c++
// 实例方法
void MyObject::LongTask() {
    Local<Object> = handle(); // this
}
```

通过handle方法就可以获取到JavaScript的this了，就可以调用JavaScript方法了。

#### C++对象析构时机

在对应的JavaScript对象销毁时析构C++对象。

#### 继承

继承见代码示例吧，是一种实用但不是很严谨的实现。

### [异步任务](4_async_task.cc)

#### 实用但不科学的方式理解Event Loop

Event Loop可以简单的用下面代码理解。

```javascript
let running = true

while(running) {
    const msgs: [] = system.getMsgs() // 非阻塞
    for (let i = 0; i < msgs.length; ++i) {
        handleMsg(msgs[i]) // 处理消息
    }
    waitForMsg() // 阻塞，如果没有新消息，进程将让出CPU
}
```

#### 长耗时任务执行

#### 非阻塞系统调用

如NodeJS提供的绝大多数IO能力，都是使用操作提供的非阻塞IO API和IO多路复用API，没有创建新线程。

#### 多线程

创建新线程以阻塞方式执行长耗时任务。Addon的实现逻辑大致为：

1. 主线程创建子线程，假设Event Loop里面没有其他要处理的消息主线程很快会睡眠。
2. 子线程将执行结果放入线程共享内存（生产者消费者模型，需要同步访问控制）。并且通过libuv的uv_async唤醒主线程，然后自行退出。
3. 主线程从线程共享内存中获得结果（需要同步访问控制），并且调用V8方法执行JavaScript回调函数将结果同步到JavaScript。

！需要注意的点！

1. C++对象的析构时机

```javascript
function foo() {
    const task = new MyTask()
    task.longTask(cb);
}

foo()
```

foo函数执行完task对象就可能被销毁了（具体取决于gc时机），很可能子线程执行完毕调用uv_async唤醒主线程时崩溃。解决方案在执行异步任务前通过ObjectWrap::Ref方法，增加内部v8::Persist句柄的引用计数，避免JavaScript对象销毁触发绑定C++对象的析构。执行完毕后调用ObjectWrap::Unref减少引用计数避免内存泄露。

2. uv_async的坑
使用uv_async.data线程间传递消息只能配合一次uv_async_send，如任务会传递多个结果，调用多次send，需要使用生产者消费者机制，不能通过uv_async的data属性传递。原因很简单，一个属性一个值，只能反复覆盖。两个线程执行状态不确定是否同步，可能主线程没有来得及访问uv_async.data而miss消息。

##### JavaScript Worker

在C++操作uv_async和生产者消费者模式实现多线程开发是复杂的。而且使用libuv的头文件就只能在指定的node版本运行，切换node版本需要使用对应node版本的头文件重新编译。NodeJS在JavaScript层也有多线程API了。Addon里面只实现阻塞调用的版本即可，大幅简化了开发难度。

使用Node Worker API调用Addon阻塞API的代码示例：

[Main Thread](5_node_worker.js)

[Worker Thread](5_node_worker_worker_thread.js)
