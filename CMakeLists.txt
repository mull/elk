cmake_minimum_required(VERSION 3.21.3)

project(
  ElkLang
  VERSION 0.0.1
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(fmt
 GIT_REPOSITORY https://github.com/fmtlib/fmt.git
 GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

#Include(FetchContent)
#FetchContent_Declare(
#  Catch2
#  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#  GIT_TAG        v2.13.6)
#FetchContent_MakeAvailable(Catch2)



# The elk library which contains the compiler & runtime
add_executable(elk 
  elk/main.cpp
  elk/gc.cpp
  elk/construct.cpp
)
target_link_libraries(elk PRIVATE fmt::fmt)
set_property(TARGET elk PROPERTY CXX_STANDARD 20)
target_compile_options(elk PRIVATE
  -Wall -Wextra -pedantic -Wconversion -Wundef
  -Wdeprecated -Wweak-vtables -Wshadow
  -Wno-gnu-zero-variadic-macro-arguments
)
