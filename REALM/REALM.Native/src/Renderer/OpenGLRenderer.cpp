#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "OpenGLRenderer.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>

#include <GL/gl.h>
#include <GL/glu.h>

#include <algorithm>
#include <cmath>

namespace
{
    float ClampFloat(float value, float minValue, float maxValue)
    {
        if (value < minValue)
            return minValue;
        if (value > maxValue)
            return maxValue;
        return value;
    }
}

OpenGLRenderer::OpenGLRenderer(QWidget* parent)
    : QOpenGLWidget(parent), world_(128, 64, 128)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    frameTimer_.setInterval(16);
    frameTimer_.setTimerType(Qt::PreciseTimer);
    connect(&frameTimer_, &QTimer::timeout, this, [this] { update(); });
    frameTimer_.start();
}

void OpenGLRenderer::SetBrushMode(BrushMode mode)
{
    brushMode_ = mode;
    brushActive_ = false;
    leftMouseDown_ = false;
    brushClock_.invalidate();
}

void OpenGLRenderer::SetEraseRadius(int radius)
{
    eraseRadius_ = std::clamp(radius, REALM::Engine::VoxelGrid::MinEraseRadius,
        REALM::Engine::VoxelGrid::MaxEraseRadius);
    brushActive_ = false;
    leftMouseDown_ = false;
    brushClock_.invalidate();
    update();
}

void OpenGLRenderer::initializeGL()
{
    if (context()->isOpenGLES() ||
        context()->format().profile() == QSurfaceFormat::CoreProfile)
    {
        qFatal("The terrain renderer requires desktop OpenGL compatibility support.");
    }

    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    frameClock_.start();
}

void OpenGLRenderer::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        window()->close();
    else
        pressedKeys_.insert(event->key());
    event->accept();
}

void OpenGLRenderer::keyReleaseEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat())
        pressedKeys_.remove(event->key());
    event->accept();
}

void OpenGLRenderer::focusOutEvent(QFocusEvent* event)
{
    pressedKeys_.clear();
    leftMouseDown_ = false;
    brushActive_ = false;
    QOpenGLWidget::focusOutEvent(event);
}

void OpenGLRenderer::mouseMoveEvent(QMouseEvent* event)
{
    mousePosition_ = event->position();
    cursorInViewport_ = rect().contains(mousePosition_.toPoint());
    event->accept();
}

void OpenGLRenderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        brushActive_ = false;
        leftMouseDown_ = true;
        setFocus();
    }
    mouseMoveEvent(event);
}

void OpenGLRenderer::mouseDoubleClickEvent(QMouseEvent* event)
{
    mousePressEvent(event);
    if (event->button() == Qt::LeftButton)
    {
        brushActive_ = brushMode_ != BrushMode::None;
        brushClock_.invalidate();
    }
}

void OpenGLRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        leftMouseDown_ = false;
        brushActive_ = false;
    }
    event->accept();
}

void OpenGLRenderer::wheelEvent(QWheelEvent* event)
{
    orbitDistance_ = ClampFloat(
        orbitDistance_ - event->angleDelta().y() / 120.0f * 3.75f, 8.0f, 260.0f);
    event->accept();
}

void OpenGLRenderer::leaveEvent(QEvent* event)
{
    cursorInViewport_ = false;
    QOpenGLWidget::leaveEvent(event);
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
    const float turnSpeed = 12.0f * deltaSeconds;
    const float zoomSpeed = 15.0f * deltaSeconds;

    if (pressedKeys_.contains(Qt::Key_A))
        yaw_ += turnSpeed;
    if (pressedKeys_.contains(Qt::Key_D))
        yaw_ -= turnSpeed;
    if (pressedKeys_.contains(Qt::Key_W))
        pitch_ -= turnSpeed;
    if (pressedKeys_.contains(Qt::Key_S))
        pitch_ += turnSpeed;
    if (pressedKeys_.contains(Qt::Key_Up))
        orbitDistance_ -= zoomSpeed;
    if (pressedKeys_.contains(Qt::Key_Down))
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

bool OpenGLRenderer::FindWorldPointUnderCursor(float& outX, float& outY, float& outZ) const
{
    if (!cursorInViewport_)
        return false;

    GLdouble model[16] = {};
    GLdouble projection[16] = {};
    GLint viewport[4] = {};

    glGetDoublev(GL_MODELVIEW_MATRIX, model);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);

    const int mouseX = static_cast<int>(mousePosition_.x() * devicePixelRatioF());
    const int mouseY = static_cast<int>(mousePosition_.y() * devicePixelRatioF());
    if (mouseX < 0 || mouseX >= viewport[2] || mouseY < 0 || mouseY >= viewport[3])
        return false;

    const int clampedMouseX = std::max(0, std::min(mouseX, viewport[2] - 1));
    const int clampedMouseY = std::max(0, std::min(mouseY, viewport[3] - 1));
    const int winY = viewport[3] - clampedMouseY - 1;

    GLfloat depth = 1.0f;
    glReadBuffer(GL_COLOR_ATTACHMENT0);
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

bool OpenGLRenderer::FindEraseCenter(Vec3& center) const
{
    if (!FindWorldPointUnderCursor(center.x, center.y, center.z) ||
        center.x < 0.0f || center.x >= world_.Width() ||
        center.z < 0.0f || center.z >= world_.Depth())
        return false;

    // A side-face hit must not place the brush center underground.
    return world_.TryGetSurfaceHeight(static_cast<int>(center.x),
        static_cast<int>(center.z), center.y);
}

void OpenGLRenderer::DrawErasePreview(const Vec3& center)
{
    glPushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glColor3f(1.0f, 0.55f, 0.15f);
    glLineWidth(1.5f);
    for (int plane = 0; plane < 3; ++plane)
    {
        glBegin(GL_LINE_LOOP);
        for (int segment = 0; segment < 64; ++segment)
        {
            const float angle = segment * 2.0f * 3.14159265f / 64.0f;
            const float a = std::cos(angle) * eraseRadius_;
            const float b = std::sin(angle) * eraseRadius_;
            if (plane == 0)
                glVertex3f(center.x + a, center.y, center.z + b);
            else if (plane == 1)
                glVertex3f(center.x + a, center.y + b, center.z);
            else
                glVertex3f(center.x, center.y + a, center.z + b);
        }
        glEnd();
    }
    glPopAttrib();
}

void OpenGLRenderer::ApplyBrushFromCursor()
{
    if (!brushActive_ || !leftMouseDown_)
        return;

    if (brushMode_ == BrushMode::Water)
    {
        EmitWaterFromCursor();
        return;
    }

    if ((brushMode_ != BrushMode::Soil && brushMode_ != BrushMode::Erase) ||
        (brushClock_.isValid() && brushClock_.elapsed() < 100))
        return;

    if (brushMode_ == BrushMode::Erase)
    {
        Vec3 center;
        if (FindEraseCenter(center) &&
            world_.TryEraseSphere(center.x, center.z, eraseRadius_))
            brushClock_.start();
        return;
    }

    float worldX = 0.0f;
    float worldY = 0.0f;
    float worldZ = 0.0f;
    if (!FindWorldPointUnderCursor(worldX, worldY, worldZ))
        return;

    const int x = static_cast<int>(std::floor(worldX));
    const int z = static_cast<int>(std::floor(worldZ));
    if (world_.TryAddSoilAboveSurface(x, z))
        brushClock_.start();
}

void OpenGLRenderer::EmitWaterFromCursor()
{
    float worldX = 0.0f;
    float worldY = 0.0f;
    float worldZ = 0.0f;
    if (!FindWorldPointUnderCursor(worldX, worldY, worldZ))
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

void OpenGLRenderer::paintGL()
{
    const float deltaSeconds = std::min(frameClock_.restart() / 1000.0f, 0.1f);
    UpdateCamera(deltaSeconds);

    world_.UpdateSimulation(1.0f / 60.0f);
    if (world_.IsTerrainDirty())
        BuildTerrainMesh();

    const int pixelWidth = std::max(1, qRound(width() * devicePixelRatioF()));
    const int pixelHeight = std::max(1, qRound(height() * devicePixelRatioF()));
    glViewport(0, 0, pixelWidth, pixelHeight);
    glClearColor(0.08f, 0.09f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, static_cast<double>(pixelWidth) / pixelHeight, 0.1, 500.0);

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

    Vec3 eraseCenter;
    const bool showErasePreview = brushMode_ == BrushMode::Erase && FindEraseCenter(eraseCenter);
    ApplyBrushFromCursor();
    if (showErasePreview)
        DrawErasePreview(eraseCenter);
}