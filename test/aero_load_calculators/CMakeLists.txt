find_package(GTest)
find_package(spdlog)

add_executable(calculator_test hybrid_force_torque_calculator_integration_test.cpp)

target_link_libraries(calculator_test PRIVATE GTest::gtest_main spdlog::spdlog aero_load_calculator core gsi shading_pipeline satellite)

include(GoogleTest)
gtest_discover_tests(calculator_test)