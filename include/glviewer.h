#ifndef GLVIEWER_H
#define GLVIEWER_H

#include <QtGui/QOpenGLTexture>

#include "openglwindow.h"
#include "geometryengine.h"

class GLviewer : public OpenGLWindow {
    Q_OBJECT
    
public:
    explicit GLviewer(const std::vector<cv::Mat *> & ctImages);

    ~GLviewer();

    void initialize();
    void render();

protected:
    void initTextures();

    void paintGL();

    void resizeGL(int width, int height);

private:
    QOpenGLShaderProgram * _program;

    int _shaderMatrix;

    GeometryEngine _geometryEngine;

    QOpenGLTexture _textureCV;
    GLuint _textureCVGL;

    std::vector<cv::Mat*> _ctImages;

signals:

public slots:

};

#endif // GLVIEWER_H
