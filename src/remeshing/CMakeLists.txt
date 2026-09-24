find_package(CGAL REQUIRED)

add_library(remeshing cgal_mesh.h cgal_mesh.cpp)

target_compile_features(remeshing PRIVATE cxx_std_20)
target_include_directories(remeshing
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<INSTALL_INTERFACE:include>
)
target_link_libraries(remeshing PRIVATE CGAL::CGAL)

add_library(Vat::remeshing ALIAS remeshing)
