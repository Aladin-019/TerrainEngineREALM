#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QStatusBar>
#include <QSurfaceFormat>
#include <QToolBar>

#include "../Renderer/OpenGLRenderer.h"

int main(int argc, char* argv[])
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::NoProfile);
    format.setDepthBufferSize(24);
    format.setSamples(0);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication application(argc, argv);

    QMainWindow window;
    window.setWindowTitle(QStringLiteral("R.E.A.L.M."));
    window.resize(1024, 600);
    auto* viewport = new OpenGLRenderer(&window);
    window.setCentralWidget(viewport);
    auto* brushToolbar = window.addToolBar(QStringLiteral("Brush"));
    brushToolbar->setMovable(false);
    auto* brushSelector = new QComboBox(brushToolbar);
    brushSelector->addItem(QStringLiteral("Navigate"), static_cast<int>(OpenGLRenderer::BrushMode::None));
    brushSelector->addItem(QStringLiteral("Soil"), static_cast<int>(OpenGLRenderer::BrushMode::Soil));
    brushSelector->addItem(QStringLiteral("Water"), static_cast<int>(OpenGLRenderer::BrushMode::Water));
    brushSelector->addItem(QStringLiteral("Erase"), static_cast<int>(OpenGLRenderer::BrushMode::Erase));
    brushSelector->setToolTip(QStringLiteral("Select a brush, then double-click, hold and drag over terrain"));
    brushToolbar->addWidget(brushSelector);
    auto* radiusLabel = new QLabel(QStringLiteral(" Erase radius: "), brushToolbar);
    brushToolbar->addWidget(radiusLabel);
    auto* radiusSelector = new QSpinBox(brushToolbar);
    radiusSelector->setRange(REALM::Engine::VoxelGrid::MinEraseRadius,
        REALM::Engine::VoxelGrid::MaxEraseRadius);
    radiusSelector->setValue(viewport->EraseRadius());
    radiusSelector->setSuffix(QStringLiteral(" voxels"));
    radiusSelector->setToolTip(QStringLiteral("Radius of the spherical eraser"));
    radiusSelector->setEnabled(false);
    radiusLabel->setBuddy(radiusSelector);
    brushToolbar->addWidget(radiusSelector);
    QObject::connect(radiusSelector, &QSpinBox::valueChanged, viewport,
        [viewport](int radius) { viewport->SetEraseRadius(radius); });
    QObject::connect(brushSelector, &QComboBox::currentIndexChanged, viewport,
        [viewport, brushSelector, radiusSelector](int) {
            const auto mode = static_cast<OpenGLRenderer::BrushMode>(brushSelector->currentData().toInt());
            viewport->SetBrushMode(mode);
            radiusSelector->setEnabled(mode == OpenGLRenderer::BrushMode::Erase);
            viewport->setFocus();
        });
    window.statusBar()->showMessage(QStringLiteral(
        "W/S: tilt | A/D: orbit | Up/Down or wheel: zoom | Brush: double-click, hold + drag | Esc: exit"));
    window.show();
    viewport->setFocus();

    return application.exec();
}