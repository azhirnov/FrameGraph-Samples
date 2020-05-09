file( GLOB_RECURSE SOURCES "*.*" )
file( GLOB_RECURSE SHADER_SOURCES "../shaderlib/*.*" )

add_library( "Samples.Utils" STATIC ${SOURCES} ${SHADER_SOURCES} )

source_group( TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES} )
source_group( TREE "${CMAKE_CURRENT_SOURCE_DIR}/.." FILES ${SHADER_SOURCES} )

target_include_directories( "Samples.Utils" PUBLIC "." )
set_property( TARGET "Samples.Utils" PROPERTY FOLDER "Samples" )

if (${FG_ENABLE_GLM})
	target_link_libraries( "Samples.Utils" PUBLIC "Scene" )
endif ()
