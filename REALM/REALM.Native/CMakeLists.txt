cmake_minimum_required(VERSION 3.21)

project(TerrainEngineREALM LANGUAGES CXX)

if(NOT WIN32)
    message(FATAL_ERROR "The current renderer requires Windows.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)

qt_add_executable(TerrainEngineREALM
    src/App/main.cpp
    src/Engine/VoxelGrid.cpp
    src/Renderer/OpenGLRenderer.cpp
)

target_include_directories(TerrainEngineREALM PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(TerrainEngineREALM PRIVATE
    Qt6::Widgets
    Qt6::OpenGLWidgets
    OpenGL::GL
    OpenGL::GLU
)

set_target_properties(TerrainEngineREALM PROPERTIES WIN32_EXECUTABLE TRUE)

add_custom_command(TARGET TerrainEngineREALM POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:TerrainEngineREALM>/platforms"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
        "$<TARGET_FILE_DIR:TerrainEngineREALM>/platforms"
    VERBATIM
)

include(CTest)
if(BUILD_TESTING)
    add_executable(VoxelGridBrushTests
        tests/VoxelGridBrushTests.cpp
        src/Engine/VoxelGrid.cpp
    )
    target_include_directories(VoxelGridBrushTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME VoxelGridBrushTests COMMAND VoxelGridBrushTests)
    
    add_executable(VoxelGridEraseTests
        tests/VoxelGridEraseTests.cpp
        src/Engine/VoxelGrid.cpp
    )
    target_include_directories(VoxelGridEraseTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME VoxelGridEraseTests COMMAND VoxelGridEraseTests)
    
    add_executable(VoxelGridSphereEraseTests
        tests/VoxelGridSphereEraseTests.cpp
        src/Engine/VoxelGrid.cpp
    )
    target_include_directories(VoxelGridSphereEraseTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME VoxelGridSphereEraseTests COMMAND VoxelGridSphereEraseTests)
endif()