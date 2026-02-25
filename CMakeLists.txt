cmake_minimum_required(VERSION 3.18)
cmake_policy(SET CMP0091 NEW)
cmake_policy(SET CMP0104 NEW)

# Declare CXX-only first so GCC 15 feature detection runs cleanly
project(orbinal VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Now enable CUDA after CXX is configured
set(CMAKE_CUDA_COMPILER      "/opt/cuda/bin/nvcc")
set(CUDAToolkit_ROOT         "/opt/cuda")
set(CUDA_TOOLKIT_ROOT_DIR    "/opt/cuda")
set(CMAKE_LIBRARY_PATH       "/opt/cuda/lib64" "/opt/cuda/lib" ${CMAKE_LIBRARY_PATH})
set(CMAKE_INCLUDE_PATH       "/opt/cuda/include" ${CMAKE_INCLUDE_PATH})
enable_language(CUDA)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# CUDA arch — auto-detect
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

find_package(Qt6 COMPONENTS Core Widgets OpenGL OpenGLWidgets QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Widgets OpenGL REQUIRED)
    set(QT_VERSION_MAJOR 5)
else()
    set(QT_VERSION_MAJOR 6)
endif()

find_package(OpenGL REQUIRED)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/GlobeWidget.cpp
    src/GeoSyncPanel.cpp
    src/SignalGraphWidget.cpp
    src/SatelliteModel.cpp
    src/RadarWidget.cpp
    src/CrtOverlay.cpp
    src/CudaCompute.cpp
    src/OrbitalKernels.cu
)

set(HEADERS
    src/MainWindow.h
    src/GlobeWidget.h
    src/GeoSyncPanel.h
    src/SignalGraphWidget.h
    src/SatelliteModel.h
    src/RadarWidget.h
    src/CrtOverlay.h
    src/ContinentData.h
    src/CudaCompute.h
    src/OrbitalKernels.cuh
)

add_executable(orbinal ${SOURCES} ${HEADERS})

# Mark .cu file as CUDA
set_source_files_properties(src/OrbitalKernels.cu PROPERTIES LANGUAGE CUDA)

# Enable CUDA in the target
set_target_properties(orbinal PROPERTIES CUDA_SEPARABLE_COMPILATION ON)

# Define CUDA enabled
target_compile_definitions(orbinal PRIVATE ORBINAL_CUDA_ENABLED)

if(QT_VERSION_MAJOR EQUAL 6)
    target_link_libraries(orbinal PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        OpenGL::GL
    )
else()
    target_link_libraries(orbinal PRIVATE
        Qt5::Core
        Qt5::Widgets
        Qt5::OpenGL
        OpenGL::GL
    )
endif()

target_include_directories(orbinal PRIVATE
    src
    /opt/cuda/include
)

target_link_directories(orbinal PRIVATE /opt/cuda/lib64 /opt/cuda/lib)
target_link_libraries(orbinal PRIVATE cudart)
