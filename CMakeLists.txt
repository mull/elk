cmake_minimum_required(VERSION 3.20)

project(
  ElkLang
  VERSION 0.0.1
  LANGUAGES CXX)

add_subdirectory(extern/fmt)
# find_package(fmt REQUIRED)

set(CXX_EXTENSIONS ON)
set(CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

add_library(elk INTERFACE)
# target_link_libraries(elk fmt::fmt)
target_include_directories(elk INTERFACE "${PROJECT_SOURCE_DIR}/elk")

set_property(TARGET elk PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)



add_executable(ElkPlay elk_play.cpp)
target_link_libraries(ElkPlay elk fmt::fmt)

set_property(TARGET ElkPlay PROPERTY 
  CXX_STANDARD 20
  CXX_STANDARD_REQUIRED ON
)