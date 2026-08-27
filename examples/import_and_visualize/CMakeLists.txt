find_package(spdlog CONFIG REQUIRED)

add_executable(import_and_visualize import_and_visualize.cpp)

target_link_libraries(import_and_visualize PRIVATE
	geometry
	spdlog::spdlog
	visualization
)