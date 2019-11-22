cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

file( GLOB_RECURSE SOURCES "*.*" )
add_library( "Samples.Utils" STATIC ${SOURCES} )
source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
target_include_directories( "Samples.Utils" PUBLIC "." )
set_property( TARGET "Samples.Utils" PROPERTY FOLDER "Samples" )

if (${FG_ENABLE_GLM})
	target_link_libraries( "Samples.Utils" PUBLIC "Scene" )
endif ()
