cmake_minimum_required(VERSION 3.20)

add_library(cx-console-chalk INTERFACE)
add_library(cx-console-chalk::cx-console-chalk ALIAS cx-console-chalk)

target_include_directories(cx-console-chalk INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(cx-console-chalk INTERFACE cxx_std_23)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS cx-console-chalk EXPORT cx-console-chalk-targets)
install(EXPORT cx-console-chalk-targets
  FILE cx-console-chalk-targets.cmake
  NAMESPACE cx-console-chalk::
  DESTINATION lib/cmake/cx-console-chalk
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cx-console-chalk-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-chalk-config.cmake"
  INSTALL_DESTINATION lib/cmake/cx-console-chalk
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-chalk-config-version.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-chalk-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-chalk-config-version.cmake"
  DESTINATION lib/cmake/cx-console-chalk
)

option(CX_CONSOLE_CHALK_BUILD_TESTS "Build cx-console-chalk tests" ON)
if(CX_CONSOLE_CHALK_BUILD_TESTS)
  add_subdirectory(tests)
endif()
