clang++ main.cc -o main
clang++ -shared -fPIC b.cc -o libb.so  -undefined dynamic_lookup

./main