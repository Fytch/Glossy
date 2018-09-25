cmake_minimum_required( VERSION 2.8.8 )
project( Glossy )

#set( CMAKE_CXX_COMPILER clang++ )
#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --target=x86_64-w64-mingw32 " )

if( NOT SFML_ROOT )
	set( SFML_ROOT "$ENV{SFML_ROOT}" )
	if( NOT SFML_ROOT )
		message( FATAL_ERROR "Please point the environment variable SFML_ROOT to the include directory of your SFML installation." )
	endif()
endif()
include_directories( "${SFML_ROOT}/include" )
link_directories( "${SFML_ROOT}/lib" )

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic-errors -std=c++14 -march=native" )
set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -ggdb -fno-omit-frame-pointer" )
set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -s -flto" )

include_directories( "./include/" )
include_directories( "./ext/json/include/" )

file( GLOB_RECURSE srcs "./src/*.cpp" )
add_library( glossy_objs OBJECT ${srcs} )

add_executable( Glossy $<TARGET_OBJECTS:glossy_objs> )
target_link_libraries( Glossy
	debug     sfml-system-d   optimized sfml-system
	debug     sfml-window-d   optimized sfml-window
	debug     sfml-graphics-d optimized sfml-graphics )
