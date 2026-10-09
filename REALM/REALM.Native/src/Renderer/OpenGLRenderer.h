#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <vector>

#include <windows.h>
#include <windowsx.h>

#include "../Engine/VoxelGrid.h"

class OpenGLRenderer
{
public:
    OpenGLRenderer(int width, int height, const std::string& title);
    ~OpenGLRenderer();

    void Run();
    void RenderFrame();
    void UpdateCamera(float deltaSeconds);

private:
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct Vertex
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float r = 0.0f;
        float g = 0.0f;
        float b = 0.0f;
    };

    bool Initialize();
    void Shutdown();
    void BuildTerrainMesh();
    void EmitWaterFromCursor();
    bool FindWorldPointUnderCursor(int mouseX, int mouseY, float& outX, float& outY, float& outZ) const;
    void AddFace(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color);
    void AddFaceRight(int x, int y, int z);
    void AddFaceLeft(int x, int y, int z);
    void AddFaceTop(int x, int y, int z);
    void AddFaceBottom(int x, int y, int z);
    void AddFaceFront(int x, int y, int z);
    void AddFaceBack(int x, int y, int z);
    static Vec3 GetColorForType(int type, float height);
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND window_ = nullptr;
    HDC deviceContext_ = nullptr;
    HGLRC renderContext_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::string title_;
    bool running_ = true;
    bool keyState_[256] = {};
    float yaw_ = 45.0f;
    float pitch_ = 15.0f;
    float orbitDistance_ = 200.0f;
    float cameraX_ = 0.0f;
    float cameraY_ = 10.0f;
    float cameraZ_ = 28.0f;
    bool emitWaterActive_ = false;
    bool leftMouseDown_ = false;
    int mouseX_ = 0;
    int mouseY_ = 0;
    REALM::Engine::VoxelGrid world_;
    std::vector<Vertex> terrainVertices_;
    std::vector<unsigned int> terrainIndices_;
};