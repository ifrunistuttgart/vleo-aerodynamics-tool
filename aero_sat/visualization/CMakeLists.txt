find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(VTK REQUIRED COMPONENTS
		CommonCore
		CommonDataModel
		FiltersCore          # <--- For vtkTubeFilter
		FiltersSources
		FiltersModeling
		RenderingCore
		RenderingOpenGL2
		InteractionStyle
		RenderingAnnotation  # <--- For vtkAxesActor
		InteractionWidgets   # <--- For vtkOrientationMarkerWidget
		RenderingFreeType    # <--- For VTK_MODULE_INIT(vtkRenderingFreeType)
)
add_library(visualization show_mesh.h show_mesh.cpp)

target_compile_features(visualization PRIVATE cxx_std_20)
target_include_directories(visualization
	PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)

target_link_libraries(visualization PUBLIC core glm::glm spdlog::spdlog)
target_link_libraries(visualization
		PRIVATE
		VTK::CommonCore
		VTK::CommonDataModel
		VTK::FiltersCore
		VTK::FiltersSources
		VTK::FiltersModeling
		VTK::RenderingCore
		VTK::RenderingOpenGL2
		VTK::InteractionStyle
		VTK::RenderingAnnotation # <--- Added
		VTK::InteractionWidgets  # <--- Added
		VTK::RenderingFreeType   # <--- Added
)

add_library(AeroSat::visualization ALIAS visualization)