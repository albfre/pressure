cmake_minimum_required(VERSION 3.14)
project(PressureOptimization CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Optimization flags
if(MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "/Od /Zi /D_DEBUG /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "/O2 /Zi /DNDEBUG")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -march=native -DNDEBUG")
endif()

# Define source files
set(SOURCES
    src/main.cpp
    src/state.cpp
)

# Define header files
set(HEADERS
    include/state.h
)

# Add the executable
add_executable(pressure_optimization ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(pressure_optimization
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable link time optimization for Release and RelWithDebInfo builds
include(CheckIPOSupported)
check_ipo_supported(RESULT supported OUTPUT error)
if(supported)
    set_property(TARGET pressure_optimization PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set_property(TARGET pressure_optimization PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
else()
    message(WARNING "IPO is not supported: ${error}")
endif()

# Add an option for address sanitizer
option(USE_SANITIZER "Build with Address Sanitizer" OFF)
if(USE_SANITIZER)
    if(MSVC)
        message(WARNING "Address Sanitizer is not supported with MSVC")
    else()
        target_compile_options(pressure_optimization PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(pressure_optimization PRIVATE -fsanitize=address)
    endif()
endif()

# Install the executable
install(TARGETS pressure_optimization DESTINATION bin)

# Install headers
install(DIRECTORY include/ DESTINATION include)