find_package(GTest)

add_executable(gsi_test test_sentman.cpp)

target_link_libraries(gsi_test PRIVATE GTest::gtest_main AeroSat::gsi AeroSat::core)

include(GoogleTest)
gtest_discover_tests(gsi_test)