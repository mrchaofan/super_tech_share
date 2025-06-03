void foo();

extern "C" {
    void func(void) { foo(); }
}
