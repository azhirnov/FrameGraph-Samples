if (TARGET "Samples.Utils" AND TARGET "DevIL-lib" AND TARGET "Assimp-lib")
	file( GLOB_RECURSE SOURCES "*.h" "*.cpp" "*.glsl" )
	add_executable( "Samples.RayTracing" ${SOURCES} )
	source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
	target_include_directories( "Samples.RayTracing" PUBLIC "." )
	set_property( TARGET "Samples.RayTracing" PROPERTY FOLDER "Samples" )
	target_link_libraries( "Samples.RayTracing" PUBLIC "Samples.Utils" )
	
	if ( TARGET "UI" )
		target_link_libraries( "Samples.RayTracing" PUBLIC "UI" )
	endif ()

	target_compile_definitions( "Samples.RayTracing" PUBLIC "FG_DATA_PATH=R\"(${CMAKE_CURRENT_SOURCE_DIR}/)\"" )
endif ()
