mkdir -p ./bin
/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 -fmodules -Wno-reorder io_test.cpp -o ./bin/io_test.o && ./bin/io_test.o