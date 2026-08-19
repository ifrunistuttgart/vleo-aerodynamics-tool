find_package(spdlog CONFIG REQUIRED)

add_executable(compute_force_and_torque compute_force_and_torque.cpp)

target_link_libraries(compute_force_and_torque PRIVATE
	gsi
	satellite
	shading_pipeline
	spdlog::spdlog
	visualization
)
