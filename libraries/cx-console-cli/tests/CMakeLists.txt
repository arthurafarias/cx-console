add_executable(cx-console-cli-tests main.cpp)
target_link_libraries(cx-console-cli-tests PRIVATE cx-console-cli::cx-console-cli)
target_compile_features(cx-console-cli-tests PRIVATE cxx_std_23)
add_test(NAME cx-console-cli-tests COMMAND cx-console-cli-tests)
