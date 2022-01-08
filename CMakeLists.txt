 
cmake_minimum_required(VERSION 3.12)

project(longeron VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_library(longeron INTERFACE)

target_include_directories(longeron INTERFACE src)

option(LONGERON_BUILD_EXAMPLES "Build Examples" OFF)
option(LONGERON_BUILD_TESTS "Build unit tests" OFF)

if (LONGERON_BUILD_EXAMPLES)
  add_subdirectory(examples)
  message(STATUS "building examples")
endif()

if (LONGERON_BUILD_TESTS)
  add_subdirectory(3rdparty)
  add_subdirectory(test)
endif()
