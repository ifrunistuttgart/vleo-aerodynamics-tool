target_sources(shading PRIVATE
	binary_shader.h
	binary_shader.cpp
        ../gl/frame_buffer.h
        ../gl/frame_buffer.cpp
)
add_subdirectory(shaders)