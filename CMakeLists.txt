cmake_minimum_required(VERSION 3.10)
project(rt-plotter-sdl)

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
	magsensor.cpp
)

set(HEADERS
    args-parsing.h
    colors.h
    fsrs.h
    PPP.h
    ppp-parsing.h
    u32_fmt_t.h
	magsensor.h
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

# SDL2 Setup
if(WIN32)
    # Path to your local SDL2 copy
    set(SDL2_DIR "${CMAKE_SOURCE_DIR}/SDL")

    # Determine the target architecture
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        # 64-bit architecture
        set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x64")
    else()
        # 32-bit architecture
        set(SDL2_LIB_DIR "${SDL2_DIR}/lib/x86")
    endif()

    # Include SDL2 headers
    include_directories(${SDL2_DIR}/include)
    
    # Link SDL2 libraries
    target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIB_DIR}/SDL2.lib
        ${SDL2_LIB_DIR}/SDL2main.lib
    )
else()
    # Find SDL2 on other platforms
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})
endif()


# Platform-specific libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} setupapi)
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()
