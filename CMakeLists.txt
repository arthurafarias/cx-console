cmake_minimum_required(VERSION 3.20)
project(cx-console VERSION 0.1.0 LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

if(NOT TARGET cxcore::cxcore)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/cx-core/CMakeLists.txt")
    set(CXCORE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(third-party/cx-core EXCLUDE_FROM_ALL)
  else()
    find_package(cxcore REQUIRED)
  endif()
endif()

add_subdirectory(libraries/cx-console-chalk)
add_subdirectory(libraries/cx-console-cli)
