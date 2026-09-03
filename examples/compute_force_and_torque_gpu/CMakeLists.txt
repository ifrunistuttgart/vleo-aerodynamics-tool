find_package(spdlog CONFIG REQUIRED)

add_executable(compute_force_and_torque_gpu compute_force_and_torque_gpu.cpp)

target_link_libraries(compute_force_and_torque_gpu PRIVATE
	satellite
	aero_load_calculator
	spdlog::spdlog
	visualization
	gsi
	AeroSat::gsi::cpu
	shading_pipeline
)
