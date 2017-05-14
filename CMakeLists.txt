# Project named after directory
get_filename_component(game ${CMAKE_CURRENT_SOURCE_DIR} NAME)
cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(${game} CXX)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

file(GLOB SRC_FILES src/*.cpp)

# Libs...
include(FindPkgConfig)

# .. SDL2
pkg_search_module(SDL2 REQUIRED sdl2)
include_directories(${SDL2_INCLUDE_DIRS})
link_libraries(${SDL2_LIBRARIES})

# include(FindSDL)
# if (SDL_FOUND)
# 	include_directories(${SDL_INCLUDE_DIR})
# 	link_libraries(${SDL_LIBRARY})
# endif()

# .. SDL2_Image
pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
include_directories(${SDL2IMAGE_INCLUDE_DIRS})
link_libraries(${SDL2IMAGE_LIBRARIES})

# .. OpenGL
include(FindOpenGL)
if (OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
	link_libraries(${OPENGL_gl_LIBRARY})
endif()

# OpenCV
find_package(OpenCV REQUIRED)
link_libraries(${OpenCV_LIBS})

add_executable(${game} ${SRC_FILES})

# Rest off flag stuff
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_BUILD_TYPE Debug)
