find_package(GTest)

add_executable(gsi_test test_sentman.cpp test_lift_sign.cpp test_backface.cpp)

target_link_libraries(gsi_test PRIVATE GTest::gtest_main Vat::gsi_models Vat::core)

# The GPU half of every model against its C++ half; needs an OpenGL 4.3 context.
add_executable(gsi_glsl_test test_glsl_matches_cpp.cpp)
target_link_libraries(gsi_glsl_test PRIVATE GTest::gtest_main Vat::gsi_models Vat::core Vat::gl)

include(GoogleTest)
gtest_discover_tests(gsi_test)
gtest_discover_tests(gsi_glsl_test)