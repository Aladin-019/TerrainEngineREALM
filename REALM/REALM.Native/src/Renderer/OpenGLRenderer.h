#pragma once

#include <QElapsedTimer>
#include <QOpenGLWidget>
#include <QPointF>
#include <QSet>
#include <QTimer>

#include <vector>

#include "../Engine/VoxelGrid.h"

class OpenGLRenderer : public QOpenGLWidget
{
public:
    enum class BrushMode
    {
        None,
        Soil,
        Water,
        Erase
    };

    explicit OpenGLRenderer(QWidget* parent = nullptr);
    void SetBrushMode(BrushMode mode);
    void SetEraseRadius(int radius);
    int EraseRadius() const { return eraseRadius_; }

protected:
    void initializeGL() override;
    void paintGL() override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void leaveEvent(QEvent* event) override;

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

    void UpdateCamera(float deltaSeconds);
    void BuildTerrainMesh();
    void EmitWaterFromCursor();
    void ApplyBrushFromCursor();
    bool FindWorldPointUnderCursor(float& outX, float& outY, float& outZ) const;
    bool FindEraseCenter(Vec3& center) const;
    void DrawErasePreview(const Vec3& center);
    void AddFace(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d, const Vec3& color);
    void AddFaceRight(int x, int y, int z);
    void AddFaceLeft(int x, int y, int z);
    void AddFaceTop(int x, int y, int z);
    void AddFaceBottom(int x, int y, int z);
    void AddFaceFront(int x, int y, int z);
    void AddFaceBack(int x, int y, int z);
    static Vec3 GetColorForType(int type, float height);
    QTimer frameTimer_;
    QElapsedTimer frameClock_;
    QElapsedTimer brushClock_;
    QSet<int> pressedKeys_;
    float yaw_ = 45.0f;
    float pitch_ = 15.0f;
    float orbitDistance_ = 200.0f;
    float cameraX_ = 0.0f;
    float cameraY_ = 10.0f;
    float cameraZ_ = 28.0f;
    BrushMode brushMode_ = BrushMode::None;
    bool brushActive_ = false;
    bool leftMouseDown_ = false;
    QPointF mousePosition_;
    bool cursorInViewport_ = false;
    int eraseRadius_ = 3;
    REALM::Engine::VoxelGrid world_;
    std::vector<Vertex> terrainVertices_;
    std::vector<unsigned int> terrainIndices_;
};