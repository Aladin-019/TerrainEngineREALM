#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>
#include <QSurfaceFormat>

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
    window.statusBar()->showMessage(QStringLiteral(
        "W/S: tilt | A/D: orbit | Up/Down or wheel: zoom | Double-click and hold: water | Esc: exit"));
    window.show();
    viewport->setFocus();

    return application.exec();
}