find_package(spdlog REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED CONFIG)

add_library(gl
	compute_shader.cpp
	compute_shader.h
	frame_buffer.cpp
	frame_buffer.h
	gl_helpers.cpp
	gl_helpers.h
	glfw_opengl_context.cpp
	glfw_opengl_context.h
	index_buffer.cpp
	index_buffer.h
	shader.cpp
	shader.h
	vertex_array.cpp
	vertex_array.h
	vertex_buffer.cpp
	vertex_buffer.h
	vertex_buffer_layout.h
	visibility_reducer.cpp
	visibility_reducer.h)

target_compile_features(gl PRIVATE cxx_std_20)
target_include_directories(gl
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)

# PUBLIC because the wrapper headers expose GL and glm types to their users.
target_link_libraries(gl PUBLIC OpenGL::GL glfw GLEW::GLEW glm::glm)
target_link_libraries(gl PRIVATE spdlog::spdlog)

add_library(Vat::gl ALIAS gl)
