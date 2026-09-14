add_executable(cx-console-chalk-tests main.cpp)
target_link_libraries(cx-console-chalk-tests PRIVATE cx-console-chalk::cx-console-chalk)
target_compile_features(cx-console-chalk-tests PRIVATE cxx_std_23)

add_test(NAME cx-console-chalk-tests COMMAND cx-console-chalk-tests)
