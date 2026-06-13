find_package(spdlog CONFIG REQUIRED)

add_executable(main main.cpp)

target_link_libraries(main PRIVATE
	gsi
	satellite
	shading_pipeline
	spdlog::spdlog
	#visualization
)