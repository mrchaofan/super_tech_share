#include <napi.h>
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS *pException)
{
    HANDLE hFile = CreateFileW(L"CrashDump.dmp", GENERIC_WRITE, 0, NULL,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo = {0};
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = pException;
    dumpInfo.ClientPointers = TRUE;

    // 生成完整内存转储（含堆栈）
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                      MiniDumpWithFullMemory, &dumpInfo, NULL, NULL);
    CloseHandle(hFile);
    return EXCEPTION_EXECUTE_HANDLER; // 终止进程
}

Napi::String HelloWorld(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    return Napi::String::New(env, "Hello World");
}

void Crash(const Napi::CallbackInfo &info)
{
    volatile int *zero = nullptr;
    *zero = 0;
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    SetUnhandledExceptionFilter(ExceptionHandler);
    exports.Set("hello", Napi::Function::New(env, HelloWorld));
    exports.Set("crash", Napi::Function::New(env, Crash));
    return exports;
}

NODE_API_MODULE(addon, Init)