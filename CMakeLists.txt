cmake_minimum_required(VERSION 3.21.3)

set(CXX_EXTENSIONS ON)
set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

project(
  ElkLang
  VERSION 0.0.1
  LANGUAGES CXX)

include(FetchContent)

# add_subdirectory(extern/fmt)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fmt)

Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.6)
FetchContent_MakeAvailable(Catch2)



# The elk library which contains the compiler & runtime
add_executable(elk elk/main.cpp)
target_link_libraries(elk PRIVATE fmt::fmt)

set_property(TARGET elk PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)
