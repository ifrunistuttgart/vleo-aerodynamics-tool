find_package(GTest)
find_package(spdlog REQUIRED CONFIG)

add_executable(satellite_test test_helpers.h test_rotatable_mesh_satellite.cpp test_static_mesh_satellite.cpp)

target_link_libraries(satellite_test PRIVATE GTest::gtest_main spdlog::spdlog AeroSat::satellite)

include(GoogleTest)
gtest_discover_tests(satellite_test)

add_subdirectory(geometries)