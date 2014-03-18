#ifndef GLVIEWER_H
#define GLVIEWER_H

#include <QtGui/QOpenGLTexture>

#include "openglwindow.h"
#include "geometryengine.h"

class GLviewer : public OpenGLWindow {
    Q_OBJECT
    
public:
    GLviewer();

    ~GLviewer();

    void initialize();
    void render();

    void loadModel(const std::vector<cv::Mat*> & ctImages);

protected:
    void initTextures();

    void paintGL();

    void resizeGL(int width, int height);

private:
    QOpenGLShaderProgram * _program;

    GeometryEngine _geometryEngine;

    QOpenGLTexture * _textureCV;

    std::vector<cv::Mat*> _ctImages;

signals:

public slots:

};

#endif // GLVIEWER_H
