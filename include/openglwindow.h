#ifndef OPENGLWINDOW_H
#define OPENGLWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLPaintDevice>

class OpenGLWindow : public QWindow, protected QOpenGLFunctions {
    Q_OBJECT
public:
    explicit OpenGLWindow(QWindow * parent = 0);
    ~OpenGLWindow();

    virtual void render(QPainter * painter);
    virtual void render();

    virtual void initialize();

    void setAnimating(bool animating);

public slots:
    void renderLater();
    void renderNow();

protected:
    bool event(QEvent * event);

    void exposeEvent(QExposeEvent *event);

private:
    bool _updatePending;
    bool _animating;

    QOpenGLContext * _context;
    QOpenGLPaintDevice * _device;
};

#endif // OPENGLWINDOW_H
