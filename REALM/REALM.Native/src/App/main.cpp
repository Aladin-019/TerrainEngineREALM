#include <QApplication>
#include <QComboBox>
#include <QMainWindow>
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
    brushSelector->setToolTip(QStringLiteral("Select a brush, then double-click, hold and drag over terrain"));
    brushToolbar->addWidget(brushSelector);
    QObject::connect(brushSelector, &QComboBox::currentIndexChanged, viewport,
        [viewport, brushSelector](int) {
            viewport->SetBrushMode(static_cast<OpenGLRenderer::BrushMode>(brushSelector->currentData().toInt()));
            viewport->setFocus();
        });
    window.statusBar()->showMessage(QStringLiteral(
        "W/S: tilt | A/D: orbit | Up/Down or wheel: zoom | Brush: double-click, hold + drag | Esc: exit"));
    window.show();
    viewport->setFocus();

    return application.exec();
}