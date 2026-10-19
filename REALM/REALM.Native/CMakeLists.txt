cmake_minimum_required(VERSION 3.16)

project(TerrainEngineREALM LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)

add_executable(TerrainEngineREALM
    src/App/main.cpp
    src/Engine/VoxelGrid.cpp
    src/Renderer/OpenGLRenderer.cpp
)

target_include_directories(TerrainEngineREALM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TerrainEngineREALM PRIVATE
    ${OPENGL_LIBRARIES}
    gdi32
    user32
)