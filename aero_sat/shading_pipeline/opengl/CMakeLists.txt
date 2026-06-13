find_package(spdlog REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(GLEW REQUIRED CONFIG)

target_sources(shading_pipeline PRIVATE
    compute_shader.cpp
    glfw_opengl_context.cpp
    gl_helpers.cpp
    index_buffer.cpp
    shader.cpp
    vertex_array.cpp
    vertex_buffer.cpp
    compute_shader.h
    glfw_opengl_context.h
    gl_helpers.h
    index_buffer.h
    shader.h
    vertex_array.h
    vertex_buffer.h
    vertex_buffer_layout.h
)

target_include_directories(shading_pipeline PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(shading_pipeline PRIVATE
    spdlog::spdlog
    OpenGL::GL
    glfw
    GLEW::GLEW
)
