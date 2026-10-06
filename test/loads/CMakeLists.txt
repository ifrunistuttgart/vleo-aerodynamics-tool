find_package(GTest)
find_package(spdlog)

add_executable(calculator_test hybrid_force_torque_calculator_integration_test.cpp)

target_link_libraries(calculator_test PRIVATE GTest::gtest_main spdlog::spdlog loads core gsi_models shading geometry)

add_executable(pixel_calculator_test pixel_force_torque_calculator_test.cpp pixel_force_torque_accuracy_test.cpp in_memory_geometry.h)

target_link_libraries(pixel_calculator_test PRIVATE GTest::gtest_main spdlog::spdlog loads core gsi_models shading geometry)

include(GoogleTest)
gtest_discover_tests(calculator_test)
gtest_discover_tests(pixel_calculator_test)