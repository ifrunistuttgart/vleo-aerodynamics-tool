find_package(GTest)
find_package(CGAL REQUIRED)

add_executable(remeshing_test test_cgal_mesh.cpp)

# cgal_mesh.h exposes CGAL types, so the test needs CGAL itself, not just Vat::remeshing.
target_link_libraries(remeshing_test PRIVATE GTest::gtest_main Vat::remeshing CGAL::CGAL)

include(GoogleTest)
gtest_discover_tests(remeshing_test)
