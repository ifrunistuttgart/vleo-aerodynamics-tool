target_sources(shading_pipeline PRIVATE
	binary_shader.h
	binary_shader.cpp
	frame_buffer.h
	frame_buffer.cpp
)
add_subdirectory(shaders)