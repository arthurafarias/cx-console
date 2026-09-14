cmake_minimum_required(VERSION 3.20)

add_library(cx-console-cli INTERFACE)
add_library(cx-console-cli::cx-console-cli ALIAS cx-console-cli)

target_include_directories(cx-console-cli INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(cx-console-cli INTERFACE cxx_std_23)
target_link_libraries(cx-console-cli INTERFACE
  cx-console-chalk::cx-console-chalk
  cxcore::cxcore
)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS cx-console-cli EXPORT cx-console-cli-targets)
install(EXPORT cx-console-cli-targets
  FILE cx-console-cli-targets.cmake
  NAMESPACE cx-console-cli::
  DESTINATION lib/cmake/cx-console-cli
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/cx-console-cli-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-cli-config.cmake"
  INSTALL_DESTINATION lib/cmake/cx-console-cli
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-cli-config-version.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-cli-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/cx-console-cli-config-version.cmake"
  DESTINATION lib/cmake/cx-console-cli
)

option(CX_CONSOLE_CLI_BUILD_TESTS "Build cx-console-cli tests" ON)
if(CX_CONSOLE_CLI_BUILD_TESTS)
  add_subdirectory(tests)
endif()
