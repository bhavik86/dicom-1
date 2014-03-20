#include <QtGui/QColor>
#include <QtGui/QScreen>
#include <QtGui/QOpenGLPixelTransferOptions>

#include <iostream>

#include "glviewer.h"

#include "opencv2/highgui/highgui.hpp"

GLviewer::GLviewer(const std::vector<cv::Mat *> & ctImages) :
    _program(0),
    _rBottom(0.1),
    _rTop(0.5),
    _alpha(0),
    _beta(0),
    _distance(10),
    _textureCV3D(QOpenGLTexture::Target3D),
    _ctImages(ctImages) {

}

GLviewer::~GLviewer() {
    _textureCV3D.release();
}

void GLviewer::initialize() {
    _program = new QOpenGLShaderProgram(this);
    _program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/vertex.glsl");
    _program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":shaders/fragment.glsl");
    _program->link();

    _shaderMatrix = _program->uniformLocation("mvpMatrix");
    _shaderTexSample = _program->uniformLocation("texSample");
    _shaderRBottom = _program->uniformLocation("rBottom");
    _shaderRTop = _program->uniformLocation("rTop");

    _count = _ctImages.size();

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initTextures();

    _geometryEngine.init(_program, _count);
}

void GLviewer::fetchMatrices() {
    _cameraTransformation.rotate(_alpha, 0, 1, 0);
    _cameraTransformation.rotate(_beta, 1, 0, 0);

    _cameraPosition = _cameraTransformation * QVector3D(0, 0, _distance);
    _cameraUpDirection = _cameraTransformation * QVector3D(0, 1, 0);

    _pMatrix.setToIdentity();
    _pMatrix.ortho(-2, 2, -2, 2, 0.001, 1000);

    _mMatrix.setToIdentity();

    _vMatrix.setToIdentity();
    _vMatrix.lookAt(_cameraPosition, QVector3D(0.0, 0.0, 0.0), _cameraUpDirection);

    _program->setUniformValue(_shaderMatrix, _pMatrix * _vMatrix * _mMatrix);
}

void GLviewer::render() {
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    _program->bind();

    fetchMatrices();

    _program->setUniformValue(_shaderTexSample, 0);
    _program->setUniformValue(_shaderRBottom, _rBottom);
    _program->setUniformValue(_shaderRTop, _rTop);

    _textureCV3D.bind();

    _geometryEngine.drawModel(_program);

    _program->release();
}

void GLviewer::initTextures() {

    cv::Mat image(*(_ctImages[0]));

    QOpenGLPixelTransferOptions pixelOptions;
    pixelOptions.setAlignment((image.step & 3) ? 1 : 4);
    pixelOptions.setRowLength(image.step1());

    int byteSizeMat = _ctImages[0]->elemSize() * _ctImages[0]->total();
    int byteSizeAll = byteSizeMat * _count;

    uchar * data = new uchar[byteSizeAll];

    for (int i = 0; i != _count; ++ i) {
        cv::flip(*(_ctImages[i]), image, 0);

        memcpy(data + byteSizeMat * i, image.data, byteSizeMat);
    }

    _textureCV3D.setSize(image.cols, image.rows, _count);
    _textureCV3D.setFormat(QOpenGLTexture::R8_UNorm);
    _textureCV3D.allocateStorage();

    _textureCV3D.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (void *) data, &pixelOptions);

    _textureCV3D.setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV3D.setMagnificationFilter(QOpenGLTexture::Linear);
    _textureCV3D.setWrapMode(QOpenGLTexture::ClampToBorder);

    _textureCV3D.generateMipMaps();
}

void GLviewer::mousePressEvent(QMouseEvent * event) {
    _lastMousePosition = event->pos();

    event->accept();
}

void GLviewer::mouseMoveEvent(QMouseEvent * event) {
    int dX = event->x() - _lastMousePosition.x();
    int dY = event->y() - _lastMousePosition.y();

    if (event->buttons() & Qt::LeftButton) {
        _alpha -= dX;
        while (_alpha < 0 ) {
            _alpha += 360;
        }
        while (_alpha >= 360) {
            _alpha -= 360;
        }

        _beta -= dY;
        while (_beta < 0 ) {
            _beta += 360;
        }
        while (_beta >= 360) {
            _beta -= 360;
        }

        renderLater();
    }

    _lastMousePosition = event->pos();

    event->accept();
}

void GLviewer::wheelEvent(QWheelEvent * event) {
    int d = event->delta();

    if (event->orientation() == Qt::Vertical) {
        if (d < 0) {
            _distance *= 1.1;
        } else if (d > 0) {
            _distance *= 0.9;
        }

        renderLater();
    }

    event->accept();
}
