# R.E.A.L.M.

R.E.A.L.M. = REALM Environment Automation & Landscape Modeling.

Sandbox for building 3D geological terrains, and automating natural processes.

## Stack
- C++ for engine, simulation, and renderer core
- OpenGL for terrain rendering and visualization
- C# for User Interface

## Native build
This project is compiled directly with MinGW and Win32 OpenGL.

```bat
cd REALM.Native
mkdir build
g++ -std=c++17 -O2 -I src src/App/main.cpp src/Engine/VoxelGrid.cpp src/Renderer/OpenGLRenderer.cpp -o build\REALM.exe -lopengl32 -lglu32 -lgdi32 -luser32
```