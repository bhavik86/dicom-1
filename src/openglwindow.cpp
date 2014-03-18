#include <QtCore/QCoreApplication>

#include <QtGui/QPainter>

#include "openglwindow.h"

OpenGLWindow::OpenGLWindow(QWindow * parent) :
    QWindow(parent),
    _updatePending(false),
    _animating(false),
    _context(0),
    _device(0) {
    setSurfaceType(QWindow::OpenGLSurface);
}

OpenGLWindow::~OpenGLWindow() {

}

void OpenGLWindow::render(QPainter * painter) {
    Q_UNUSED(painter);
}

void OpenGLWindow::initialize() {

}

void OpenGLWindow::render() {
    if (!_device)
        _device = new QOpenGLPaintDevice;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    _device->setSize(size());

    QPainter painter(_device);
    render(&painter);
}

void OpenGLWindow::renderLater() {
    if (!_updatePending) {
        _updatePending = true;
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool OpenGLWindow::event(QEvent * event) {
    switch (event->type()) {
    case QEvent::UpdateRequest:
        _updatePending = false;
        renderNow();
        return true;
    default:
        return QWindow::event(event);
    }
}

void OpenGLWindow::exposeEvent(QExposeEvent *event) {
    Q_UNUSED(event);

    if (isExposed())
        renderNow();
}

void OpenGLWindow::renderNow() {
    if (!isExposed())
        return;

    bool needsInitialize = false;

    if (!_context) {
        _context = new QOpenGLContext(this);
        _context->setFormat(requestedFormat());
        _context->create();

        needsInitialize = true;
    }

    _context->makeCurrent(this);

    if (needsInitialize) {
        initializeOpenGLFunctions();
        initialize();
    }

    render();

    _context->swapBuffers(this);

    if (_animating)
        renderLater();
}

void OpenGLWindow::setAnimating(bool animating) {
    _animating = animating;

    if (animating)
        renderLater();
}
