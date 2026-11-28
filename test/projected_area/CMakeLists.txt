find_package(GTest)

add_executable(
    projected_area_test
    test_projected_area_gsi_model.cpp
    test_pixel_projected_area.cpp
)
target_include_directories(
    projected_area_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../loads
)
target_link_libraries(
    projected_area_test
    PRIVATE GTest::gtest_main Vat::projected_area Vat::core
)

include(GoogleTest)
gtest_discover_tests(projected_area_test)
