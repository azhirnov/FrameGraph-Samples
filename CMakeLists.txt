cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

project( "FrameGraph-Samples" LANGUAGES C CXX )
set_property( GLOBAL PROPERTY USE_FOLDERS ON )

# this options required for samples
set( FG_ENABLE_ASSIMP ON CACHE INTERNAL "" FORCE )
set( FG_ENABLE_GLM ON CACHE INTERNAL "" FORCE )
set( FG_ENABLE_DEVIL ON CACHE INTERNAL "" FORCE )
set( FG_ENABLE_STB ON CACHE BOOL "" FORCE )
set( FG_ENABLE_TINYOBJ ON CACHE BOOL "" FORCE )
set( FG_ENABLE_IMGUI ON CACHE BOOL "" FORCE )

add_subdirectory( "FrameGraph" )

set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${MAIN_BINARY_DIR}" )

# prepare for external projects
if (TRUE)
	include( ExternalProject )

	set( FG_LIST_SEPARATOR "|" )
	string( REPLACE ";" "${FG_LIST_SEPARATOR}" FG_EXTERNAL_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" )
	set( FG_BUILD_TARGET_FLAGS 	"-DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}"
								"-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
								"-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
								"-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
								"-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}"
								"-DCMAKE_STATIC_LINKER_FLAGS=${CMAKE_STATIC_LINKER_FLAGS}"
								"-DCMAKE_SHARED_LINKER_FLAGS=${CMAKE_SHARED_LINKER_FLAGS}" )

	foreach ( CONFIG ${CMAKE_CONFIGURATION_TYPES} )
		string( TOUPPER ${CONFIG} OUT_CONFIG )

		set( FG_BUILD_TARGET_FLAGS
			"${FG_BUILD_TARGET_FLAGS}"
			"-DCMAKE_C_FLAGS_${OUT_CONFIG}=${CMAKE_C_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_CXX_FLAGS_${OUT_CONFIG}=${CMAKE_CXX_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_EXE_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_STATIC_LINKER_FLAGS_${OUT_CONFIG}}"
			"-DCMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}=${CMAKE_SHARED_LINKER_FLAGS_${OUT_CONFIG}}" )
	endforeach ()
endif ()

include( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/download_stb.cmake" )
include( "${CMAKE_CURRENT_SOURCE_DIR}/cmake/download_tinyobj.cmake" )

add_subdirectory( "samples/utils" )
add_subdirectory( "samples/clouds" )
add_subdirectory( "samples/particles" )
add_subdirectory( "samples/planet" )
add_subdirectory( "samples/ray_tracing" )
add_subdirectory( "samples/shadertoy" )
add_subdirectory( "samples/vulkan" )
