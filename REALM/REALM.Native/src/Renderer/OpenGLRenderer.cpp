#include "OpenGLRenderer.h"

#include <GL/gl.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace
{
    OpenGLRenderer* g_rendererInstance = nullptr;

    float ClampFloat(float value, float minValue, float maxValue)
    {
        if (value < minValue)
            return minValue;
        if (value > maxValue)
            return maxValue;
        return value;
    }
}

// Window procedure callback attached to and used by window instance (see Initialize())
LRESULT CALLBACK OpenGLRenderer::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (g_rendererInstance && wParam < 256)
            g_rendererInstance->keyState_[wParam] = true;
        if (wParam == VK_ESCAPE)
            PostQuitMessage(0);
        return 0;
    case WM_KEYUP:
        if (g_rendererInstance && wParam < 256)
            g_rendererInstance->keyState_[wParam] = false;
        return 0;
    case WM_MOUSEMOVE:
        if (g_rendererInstance)
        {
            g_rendererInstance->mouseX_ = GET_X_LPARAM(lParam);
            g_rendererInstance->mouseY_ = GET_Y_LPARAM(lParam);
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (g_rendererInstance)
            g_rendererInstance->leftMouseDown_ = true;
        return 0;
    case WM_LBUTTONDBLCLK:
        if (g_rendererInstance)
        {
            g_rendererInstance->leftMouseDown_ = true;
            g_rendererInstance->emitWaterActive_ = true;
            g_rendererInstance->mouseX_ = GET_X_LPARAM(lParam);
            g_rendererInstance->mouseY_ = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
        }
        return 0;
    case WM_LBUTTONUP:
        if (g_rendererInstance)
        {
            g_rendererInstance->leftMouseDown_ = false;
            g_rendererInstance->emitWaterActive_ = false;
            ReleaseCapture();
        }
        return 0;
    case WM_CAPTURECHANGED:
        if (g_rendererInstance)
        {
            g_rendererInstance->leftMouseDown_ = false;
            g_rendererInstance->emitWaterActive_ = false;
        }
        return 0;
    case WM_MOUSEWHEEL:
        if (g_rendererInstance)
        {
            const short wheelDelta = static_cast<short>(HIWORD(wParam));
            g_rendererInstance->orbitDistance_ -= wheelDelta / 120.0f * 2.5f;
            g_rendererInstance->orbitDistance_ = std::max(8.0f, std::min(g_rendererInstance->orbitDistance_, 80.0f));
        }
        return 0;
    default:
        // defualt windows procedure
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
}

OpenGLRenderer::OpenGLRenderer(int width, int height, const std::string& title)
    : width_(width), height_(height), title_(title), world_(128, 64, 128)
{
    g_rendererInstance = this;

    if (!Initialize())
    {
        throw std::runtime_error("Failed to initialize native OpenGL renderer.");
    }
}

OpenGLRenderer::~OpenGLRenderer()
{
    Shutdown();
}

bool OpenGLRenderer::Initialize()
{
    // Program instance handle
    HINSTANCE instance = GetModuleHandle(nullptr);

    // Define window class
    WNDCLASSA wc = {};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = WindowProc; // register window procedure callback
    wc.hInstance = instance;     // link program
    wc.hbrBackground = nullptr;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "GeoSandboxNativeWindowClass";

    // Register window class
    if (!RegisterClassA(&wc))
    {
        return false;
    }

    // Create window and store handle to it => HWND window_
    window_ = CreateWindowExA(
        0,
        "GeoSandboxNativeWindowClass",
        title_.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width_,
        height_,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!window_)
    {
        return false;
    }

    // Get device context for window
    deviceContext_ = GetDC(window_);
    if (!deviceContext_)
    {
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Define pixel format descriptor type
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.iLayerType = PFD_MAIN_PLANE;

    // Get pixel format from current setup and attach it to device context
    int pixelFormat = ChoosePixelFormat(deviceContext_, &pfd);
    if (pixelFormat == 0 || !SetPixelFormat(deviceContext_, pixelFormat, &pfd))
    {
        ReleaseDC(window_, deviceContext_);
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Create window render context from window device context 
    renderContext_ = wglCreateContext(deviceContext_);
    if (!renderContext_)
    {
        ReleaseDC(window_, deviceContext_);
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    // Bind the OpenGL rendering context to the window’s device
    // context on the current thread
    if (!wglMakeCurrent(deviceContext_, renderContext_))
    {
        wglDeleteContext(renderContext_);
        ReleaseDC(window_, deviceContext_);
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }

    ShowWindow(window_, SW_SHOW);
    UpdateWindow(window_);

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    return true;
}

void OpenGLRenderer::Shutdown()
{
    if (renderContext_)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(renderContext_);
        renderContext_ = nullptr;
    }

    if (deviceContext_ && window_)
    {
        ReleaseDC(window_, deviceContext_);
        deviceContext_ = nullptr;
    }

    if (window_)
    {
        DestroyWindow(window_);
        window_ = nullptr;
    }

    UnregisterClassA("GeoSandboxNativeWindowClass", GetModuleHandle(nullptr));
}

void OpenGLRenderer::BuildTerrainMesh()
{
    terrainVertices_.clear();
    terrainIndices_.clear();

    for (int x = 0; x < world_.Width(); ++x)
    {
        for (int y = 0; y < world_.Height(); ++y)
        {
            for (int z = 0; z < world_.Depth(); ++z)
            {
                const auto& voxel = world_.GetVoxel(x, y, z);
                if (!voxel.IsVisible)
                    continue;

                if (!world_.IsVisible(x + 1, y, z))
                    AddFaceRight(x, y, z);
                if (!world_.IsVisible(x - 1, y, z))
                    AddFaceLeft(x, y, z);
                if (!world_.IsVisible(x, y + 1, z))
                    AddFaceTop(x, y, z);
                if (!world_.IsVisible(x, y - 1, z))
                    AddFaceBottom(x, y, z);
                if (!world_.IsVisible(x, y, z + 1))
                    AddFaceFront(x, y, z);
                if (!world_.IsVisible(x, y, z - 1))
                    AddFaceBack(x, y, z);
            }
        }
    }

    world_.MarkTerrainClean();
}

void OpenGLRenderer::AddFace(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color)
{
    const unsigned int indexOffset = static_cast<unsigned int>(terrainVertices_.size());

    terrainVertices_.push_back({a.x, a.y, a.z, color.x, color.y, color.z});
    terrainVertices_.push_back({b.x, b.y, b.z, color.x, color.y, color.z});
    terrainVertices_.push_back({c.x, c.y, c.z, color.x, color.y, color.z});
    terrainVertices_.push_back({d.x, d.y, d.z, color.x, color.y, color.z});

    terrainIndices_.push_back(indexOffset + 0);
    terrainIndices_.push_back(indexOffset + 1);
    terrainIndices_.push_back(indexOffset + 2);
    terrainIndices_.push_back(indexOffset + 0);
    terrainIndices_.push_back(indexOffset + 2);
    terrainIndices_.push_back(indexOffset + 3);
}

void OpenGLRenderer::AddFaceRight(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            {static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1)},
            color);
}

void OpenGLRenderer::AddFaceLeft(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z + 1)},
            {static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            {static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
            color);
}

void OpenGLRenderer::AddFaceTop(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            {static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            color);
}

void OpenGLRenderer::AddFaceBottom(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z + 1)},
            {static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1)},
            {static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z)},
            {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
            color);
}

void OpenGLRenderer::AddFaceFront(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x), static_cast<float>(y), static_cast<float>(z + 1)},
            {static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z + 1)},
            {static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z + 1)},
            color);
}

void OpenGLRenderer::AddFaceBack(int x, int y, int z)
{
    const auto voxel = world_.GetVoxel(x, y, z);
    const auto color = GetColorForType(static_cast<int>(voxel.Material), static_cast<float>(y) / static_cast<float>(world_.Height()));
    AddFace({static_cast<float>(x + 1), static_cast<float>(y), static_cast<float>(z)},
            {static_cast<float>(x + 1), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x), static_cast<float>(y + 1), static_cast<float>(z)},
            {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)},
            color);
}

OpenGLRenderer::Vec3 OpenGLRenderer::GetColorForType(int type, float height)
{
    switch (type)
    {
    case 0:
        return {0.12f + height * 0.04f, 0.38f + height * 0.02f, 0.18f};
    case 1:
        return {0.72f, 0.64f, 0.36f};
    case 4:
        return {0.20f, 0.48f, 0.90f};
    default:
        return {0.42f, 0.44f, 0.46f};
    }
}

void OpenGLRenderer::UpdateCamera(float deltaSeconds)
{
    const float turnSpeed = 8.0f * deltaSeconds;
    const float zoomSpeed = 5.0f * deltaSeconds;

    if (keyState_['A'] || keyState_['a'])
        yaw_ += turnSpeed;
    if (keyState_['D'] || keyState_['d'])
        yaw_ -= turnSpeed;
    if (keyState_['W'] || keyState_['w'])
        pitch_ -= turnSpeed;
    if (keyState_['S'] || keyState_['s'])
        pitch_ += turnSpeed;
    if (keyState_[VK_UP])
        orbitDistance_ -= zoomSpeed;
    if (keyState_[VK_DOWN])
        orbitDistance_ += zoomSpeed;

    orbitDistance_ = ClampFloat(orbitDistance_, 8.0f, 260.0f);
    pitch_ = ClampFloat(pitch_, 10.0f, 80.0f);

    const float yawRad = yaw_ * 3.14159265f / 180.0f;
    const float pitchRad = pitch_ * 3.14159265f / 180.0f;

    const float targetX = static_cast<float>(world_.Width()) * 0.5f;
    const float targetY = static_cast<float>(world_.Height()) * 0.35f;
    const float targetZ = static_cast<float>(world_.Depth()) * 0.5f;

    const float radius = orbitDistance_;
    cameraX_ = targetX + std::cos(yawRad) * std::cos(pitchRad) * radius;
    cameraY_ = targetY + std::sin(pitchRad) * radius + 6.0f;
    cameraZ_ = targetZ + std::sin(yawRad) * std::cos(pitchRad) * radius;
}

void OpenGLRenderer::Run()
{
    MSG message = {};
    DWORD lastTick = GetTickCount();

    while (running_)
    {
        while (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT)
            {
                running_ = false;
                break;
            }

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        if (running_)
        {
            const DWORD currentTick = GetTickCount();
            const float deltaSeconds = (currentTick - lastTick) / 1000.0f;
            lastTick = currentTick;

            UpdateCamera(deltaSeconds);
            RenderFrame();
            Sleep(16);
        }
    }
}

bool OpenGLRenderer::FindWorldPointUnderCursor(int mouseX, int mouseY, float& outX, float& outY, float& outZ) const
{
    GLdouble model[16] = {};
    GLdouble projection[16] = {};
    GLint viewport[4] = {};

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    const int clampedMouseX = std::max(0, std::min(mouseX, viewport[2] - 1));
    const int clampedMouseY = std::max(0, std::min(mouseY, viewport[3] - 1));
    const int winY = viewport[3] - clampedMouseY - 1;

    GLfloat depth = 1.0f;
    glReadBuffer(GL_BACK);
    glReadPixels(clampedMouseX, winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    if (depth >= 0.9999f)
        return false;

    GLdouble worldX = 0.0;
    GLdouble worldY = 0.0;
    GLdouble worldZ = 0.0;
    if (gluUnProject(static_cast<GLdouble>(clampedMouseX),
                     static_cast<GLdouble>(winY),
                     static_cast<GLdouble>(depth),
                     model,
                     projection,
                     viewport,
                     &worldX,
                     &worldY,
                     &worldZ) == GL_FALSE)
    {
        return false;
    }

    outX = static_cast<float>(worldX);
    outY = static_cast<float>(worldY);
    outZ = static_cast<float>(worldZ);
    return true;
}

void OpenGLRenderer::EmitWaterFromCursor()
{
    if (!emitWaterActive_ || !leftMouseDown_)
        return;

    float worldX = 0.0f;
    float worldY = 0.0f;
    float worldZ = 0.0f;
    if (!FindWorldPointUnderCursor(mouseX_, mouseY_, worldX, worldY, worldZ))
        return;

    const int centerX = std::max(1, std::min(world_.Width() - 2, static_cast<int>(std::floor(worldX))));
    const int centerZ = std::max(1, std::min(world_.Depth() - 2, static_cast<int>(std::floor(worldZ))));
    const int spawnY = std::max(1, std::min(world_.Height() - 2, static_cast<int>(std::floor(worldY)) + 2));

    for (int ox = 0; ox <= 0; ++ox)
    {
        for (int oz = 0; oz <= 0; ++oz)
        {
            for (int oy = 0; oy < 1; ++oy)
            {
                world_.TrySpawnWater(centerX + ox, spawnY + oy, centerZ + oz);
            }
        }
    }
}

void OpenGLRenderer::RenderFrame()
{
    if (!window_ || !deviceContext_ || !renderContext_)
    {
        return;
    }

    wglMakeCurrent(deviceContext_, renderContext_);

    world_.UpdateSimulation(1.0f / 60.0f);
    if (world_.IsTerrainDirty())
        BuildTerrainMesh();

    glViewport(0, 0, width_, height_);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<double>(width_) / static_cast<double>(height_), 0.1, 500.0);

    const float targetX = static_cast<float>(world_.Width()) * 0.5f;
    const float targetY = static_cast<float>(world_.Height()) * 0.35f;
    const float targetZ = static_cast<float>(world_.Depth()) * 0.5f;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cameraX_, cameraY_, cameraZ_,
              targetX, targetY, targetZ,
              0.0f, 1.0f, 0.0f);

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < terrainIndices_.size(); ++i)
    {
        const auto& vertex = terrainVertices_[terrainIndices_[i]];
        glColor3f(vertex.r, vertex.g, vertex.b);
        glVertex3f(vertex.x, vertex.y, vertex.z);
    }
    glEnd();

    EmitWaterFromCursor();

    SwapBuffers(deviceContext_);
}