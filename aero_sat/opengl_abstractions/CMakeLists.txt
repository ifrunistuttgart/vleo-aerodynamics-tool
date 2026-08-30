find_package(spdlog REQUIRED CONFIG)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
find_package(GLEW REQUIRED)
find_package(VTK REQUIRED COMPONENTS
    CommonCore
    CommonDataModel
    ImagingCore
    RenderingCore
    RenderingOpenGL2
    InteractionStyle
    RenderingFreeType
)

add_library(opengl_abstractions
        compute_shader.cpp
        glfw_opengl_context.cpp
        gl_helpers.cpp
        index_buffer.cpp
        shader.cpp
        vertex_array.cpp
        vertex_buffer.cpp
        compute_shader.h
        frame_buffer.cpp
        texture_2d.cpp
        glfw_opengl_context.h
        gl_helpers.h
        index_buffer.h
        shader.h
        vertex_array.h
        vertex_buffer.h
        vertex_buffer_layout.h
        frame_buffer.h
        texture_2d.h
)

target_compile_features(opengl_abstractions PRIVATE cxx_std_20)

target_include_directories(opengl_abstractions PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(opengl_abstractions
    PUBLIC
        OpenGL::GL
        GLEW::GLEW
        glfw
    PRIVATE
        spdlog::spdlog
        VTK::CommonCore
        VTK::CommonDataModel
        VTK::ImagingCore
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::InteractionStyle
        VTK::RenderingFreeType
)

add_library(AeroSat::opengl_abstractions ALIAS opengl_abstractions)
