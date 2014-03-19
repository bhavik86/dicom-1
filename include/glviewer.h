#ifndef GLVIEWER_H
#define GLVIEWER_H

#include <QtGui/QOpenGLTexture>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

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

    void mousePressEvent(QMouseEvent * event);
    void mouseMoveEvent(QMouseEvent * event);
    void wheelEvent(QWheelEvent * event);

private:
    QOpenGLShaderProgram * _program;

    int _shaderMatrix;
    int _texSample;

    int _count;

    float _alpha;
    float _beta;
    float _distance;

    QMatrix4x4 _mMatrix;
    QMatrix4x4 _vMatrix;
    QMatrix4x4 _pMatrix;
    QMatrix4x4 _cameraTransformation;

    QVector3D _cameraPosition;
    QVector3D _cameraUpDirection;

    QPoint _lastMousePosition;

    GeometryEngine _geometryEngine;

    QOpenGLTexture _textureCV;
    QOpenGLTexture _textureCV3D;

    QVector<QOpenGLTexture*>_textureCVVector;

    GLuint _textureCVGL;

    std::vector<cv::Mat*> _ctImages;

    void inline fetchMatrices();
signals:

public slots:

};

#endif // GLVIEWER_H
