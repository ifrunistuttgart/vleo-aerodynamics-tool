find_package(GTest)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED CONFIG)

add_executable(gl_test test_texture_2d.cpp)

target_link_libraries(gl_test PRIVATE
	GTest::gtest_main
	OpenGL::GL
	glfw
	GLEW::GLEW
	glm::glm
	Vat::gl)

include(GoogleTest)
gtest_discover_tests(gl_test)
