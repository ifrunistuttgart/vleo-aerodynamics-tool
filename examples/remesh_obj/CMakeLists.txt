find_package(spdlog CONFIG REQUIRED)

add_executable(remesh_obj remesh_obj.cpp)

target_link_libraries(remesh_obj PRIVATE
	geometry
	remeshing
	shading
	spdlog::spdlog
)
