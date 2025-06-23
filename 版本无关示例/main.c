#include <stdio.h>
#include <dlfcn.h>
#ifdef USE_V2
#include "v2/api.c"
#else
#include "v1/api.c"
#endif

typedef void (*Func)(void);

int main () {
    void* handle = dlopen("./libd.dylib", RTLD_LAZY);
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