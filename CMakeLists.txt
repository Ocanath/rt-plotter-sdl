cmake_minimum_required(VERSION 3.10)
project(CrossPlatformProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(${PROJECT_SOURCE_DIR})

# Source files common to both platforms
set(SOURCES
    args-parsing.cpp
    colors.cpp
    fsrs.cpp
    main.cpp
    PPP.cpp
    ppp-parsing.cpp
)

set(HEADERS
    args-parsing.h
    colors.h
    fsrs.h
    PPP.h
    ppp-parsing.h
    u32_fmt_t.h
)

# Platform-specific source files
if(WIN32)
    list(APPEND SOURCES winserial.cpp)
    list(APPEND HEADERS winserial.h)
    add_definitions(-DPLATFORM_WINDOWS)
elseif(UNIX)
    list(APPEND SOURCES linux-serial.cpp)
    list(APPEND HEADERS linux-serial.h)
    add_definitions(-DPLATFORM_LINUX)
endif()

# Add executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Find SDL2 library
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} setupapi)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()
