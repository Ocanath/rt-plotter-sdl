cmake_minimum_required(VERSION 3.7)
project(rt-sdl-plotter)
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
add_executable(rt-sdl-plotter main.cpp
	PPP.cpp
	colors.cpp
	args-parsing.cpp
)
target_link_libraries(rt-sdl-plotter ${SDL2_LIBRARIES})