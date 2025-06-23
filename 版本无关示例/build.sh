clang main.c -o main_v1
clang -DUSE_V2 main.c -o main_v2
clang -shared -fPIC d.c -o libd.dylib  -undefined dynamic_lookup

./main_v1
./main_v2
