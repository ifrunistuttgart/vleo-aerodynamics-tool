find_package(spdlog CONFIG REQUIRED)

add_executable(compute_force_and_torque_pixel compute_force_and_torque_pixel.cpp)

target_link_libraries(compute_force_and_torque_pixel PRIVATE
	gsi_models
	geometry
	loads
	spdlog::spdlog
)
