cmake_minimum_required(VERSION 3.0.0)
project(SPH)

find_package(GLEW REQUIRED)
find_package(GLUT REQUIRED)
find_package(CUDA 6.5 REQUIRED)
find_package(OpenGL REQUIRED)

if (UNIX)
    find_package(X11 REQUIRED)
endif (UNIX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

if (UNIX)
    message("X11_FOUND: ${X11_FOUND}")
endif (UNIX)

include_directories(pch)
include_directories(external)

file(GLOB APP_SOURCES ./App/*.cpp)
file(GLOB GRAPHICS_SOURCES ./Graphics/*.cpp)
file(GLOB PCH_SOURCES ./pch/*.cpp)
file(GLOB SPH_SOURCES ./SPH/*.cpp)
file(GLOB EXTERNAL_SOURCES external/*.cpp)
set(CUDA_SOURCES CUDA/radixsort.cu CUDA/System.cu)

CUDA_ADD_EXECUTABLE(SPH ${APP_SOURCES} ${GRAPHICS_SOURCES} 
    ${PCH_SOURCES} ${SPH_SOURCES} ${EXTERNAL_SOURCES} ${CUDA_SOURCES})

if(X11_FOUND)
    target_link_libraries(SPH ${X11_LIBRARIES} GLEW::GLEW GLUT::GLUT ${OPENGL_LIBRARIES})
else()
    target_link_libraries(SPH ${GLEW_LIBRARIES} ${GLUT_LIBRARY} ${OPENGL_LIBRARIES})
endif()
