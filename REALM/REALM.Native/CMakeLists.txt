cmake_minimum_required(VERSION 3.16)

project(GeoSandboxNative LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(GeoSandboxNative
    src/App/main.cpp
    src/Engine/VoxelGrid.cpp
    src/Renderer/OpenGLRenderer.cpp
)

target_include_directories(GeoSandboxNative PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GeoSandboxNative PRIVATE
    OpenGL::GL
    glfw
)
