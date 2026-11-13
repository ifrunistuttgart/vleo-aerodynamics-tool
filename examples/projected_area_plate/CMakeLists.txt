find_package(spdlog CONFIG REQUIRED)

add_executable(projected_area_plate projected_area_plate.cpp)

target_link_libraries(projected_area_plate PRIVATE
	gsi_models
	geometry
	shading
	loads
	spdlog::spdlog
)
