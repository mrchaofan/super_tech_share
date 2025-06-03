#include <iostream>
#include <dlfcn.h>

typedef void (*Func)(void);

void foo()
{
    std::cout << "Hello" << std::endl;
}

int main () {
    void* handle = dlopen("./libb.so", RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error: %s\n", dlerror());
        return 1;
    }

    // 获取函数指针
    Func func = (Func) dlsym(handle, "func");

    // 检查符号是否存在
    char* error = dlerror();
    if (error) {
        fprintf(stderr, "Symbol error: %s\n", error);
        dlclose(handle);
        return 1;
    }

    func();

    dlclose(handle);
    return 0;
}