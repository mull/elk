mkdir -p ./bin
/opt/homebrew/opt/llvm/bin/clang++ -std=c++20 -fmodules -Wno-reorder main.cpp -o ./bin/main.o && ./bin/main.o