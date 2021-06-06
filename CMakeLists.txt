cmake_minimum_required(VERSION 3.20)

project(
  ElkLang
  VERSION 0.0.1
  LANGUAGES CXX)

add_subdirectory(extern/fmt)
Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.6)
FetchContent_MakeAvailable(Catch2)


set(CXX_EXTENSIONS ON)
set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

# The elk library which contains the compiler & runtime
add_library(elk INTERFACE)
target_link_libraries(elk INTERFACE fmt::fmt)
target_include_directories(elk INTERFACE "${PROJECT_SOURCE_DIR}/elk")

set_property(TARGET elk PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

# Unit tests
add_executable(test
  test/type_relations.cpp
  test/graph.cpp
)
target_link_libraries(test elk Catch2::Catch2)
set_property(TARGET test PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

# Playground file
add_executable(ElkPlay elk_play.cpp)
target_link_libraries(ElkPlay elk fmt::fmt)

set_property(TARGET ElkPlay PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)

