find_package(GTest)
find_package(CGAL REQUIRED)

add_executable(remeshing_test test_cgal_mesh.cpp test_repair.cpp)

# cgal_mesh.h exposes CGAL types, so the test needs CGAL itself, not just Vat::remeshing.
target_link_libraries(remeshing_test PRIVATE GTest::gtest_main Vat::remeshing CGAL::CGAL)
# Shares GetTestDataPath and the geometry comparisons with the geometry tests.
target_include_directories(remeshing_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../geometry)

include(GoogleTest)
gtest_discover_tests(remeshing_test)
