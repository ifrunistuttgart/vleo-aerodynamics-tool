target_sources(shading_pipeline PRIVATE
        cop_shader.h
        cop_shader.cpp
        frame_buffer.h
        frame_buffer.cpp
)
add_subdirectory(shaders)