find_package(GTest)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(GLEW REQUIRED CONFIG)
find_package(glm REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)

add_executable(shading_pipeline_test unit_test_pipeline.cpp test_binary_shader.cpp integration_test_pipeline.cpp)

target_link_libraries(shading_pipeline_test PRIVATE
	GTest::gtest_main
	spdlog::spdlog
	OpenGL::GL
	glfw
	GLEW::GLEW
	glm::glm
	shading_pipeline
	core
	satellite)

include(GoogleTest)
gtest_discover_tests(shading_pipeline_test)

add_subdirectory(geometries)