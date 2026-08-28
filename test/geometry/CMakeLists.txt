find_package(GTest)
find_package(spdlog REQUIRED CONFIG)

add_executable(geometry_test test_helpers.h test_mesh_grouping.cpp test_rotatable_mesh_geometry.cpp test_static_mesh_geometry.cpp)

target_link_libraries(geometry_test PRIVATE GTest::gtest_main spdlog::spdlog AeroSat::geometry)

include(GoogleTest)
gtest_discover_tests(geometry_test)

add_subdirectory(geometries)