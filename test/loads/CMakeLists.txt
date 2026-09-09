find_package(GTest)
find_package(spdlog)

add_executable(calculator_test hybrid_force_torque_calculator_integration_test.cpp)

target_link_libraries(calculator_test PRIVATE GTest::gtest_main spdlog::spdlog loads core gsi_models shading geometry)

include(GoogleTest)
gtest_discover_tests(calculator_test)