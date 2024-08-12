cmake_minimum_required(VERSION 3.14)
project(PressureOptimization CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Define source files
set(SOURCES
    src/main.cpp
    src/state.cpp
    src/solver.cpp
)

# Define header files
set(HEADERS
    include/state.h
    include/solver.h
)

# Check if we're building for WebAssembly
if(EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_malloc','_free']")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s ALLOW_MEMORY_GROWTH=1 --bind")

    # Add the library
    add_library(pressure_optimization STATIC ${SOURCES} ${HEADERS})

    add_executable(pressure_optimization_wasm src/wasm_wrapper.cpp)
    target_link_libraries(pressure_optimization_wasm PRIVATE pressure_optimization)

    add_custom_command(TARGET pressure_optimization_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/pressure_optimization_wasm.js
        ${CMAKE_CURRENT_SOURCE_DIR}/web/
        COMMAND ${CMAKE_COMMAND} -E copy 
        ${CMAKE_CURRENT_BINARY_DIR}/pressure_optimization_wasm.wasm
        ${CMAKE_CURRENT_SOURCE_DIR}/web/
    )
else()
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

    # Add the executable
    add_executable(pressure_optimization ${SOURCES} ${HEADERS})

    # Find OpenMP package
    find_package(OpenMP REQUIRED)

    # Link libraries and add OpenMP flags
    target_link_libraries(pressure_optimization PRIVATE OpenMP::OpenMP_CXX)

    # Enable link time optimization for Release and RelWithDebInfo builds
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        set_property(TARGET pressure_optimization PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
        set_property(TARGET pressure_optimization PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    else()
        message(WARNING "IPO is not supported: ${error}")
    endif()
endif()

# Include directories
target_include_directories(pressure_optimization
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Install the executable
install(TARGETS pressure_optimization DESTINATION bin)

# Install headers
install(DIRECTORY include/ DESTINATION include)