#include <QtGui/QColor>
#include <QtGui/QScreen>
#include <QtGui/QOpenGLPixelTransferOptions>

#include "glviewer.h"

#include "opencv2/highgui/highgui.hpp"

GLviewer::GLviewer(const std::vector<cv::Mat *> & ctImages) :
    _program(0),
    _alpha(0),
    _beta(0),
    _distance(10),
    _textureCV(QOpenGLTexture::Target2D),
    _ctImages(ctImages) {

}

GLviewer::~GLviewer() {
    _textureCV.release();
}

void GLviewer::initialize() {
    _program = new QOpenGLShaderProgram(this);
    _program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/vertex.glsl");
    _program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":shaders/fragment.glsl");
    _program->link();

    _shaderMatrix = _program->uniformLocation("mvpMatrix");
    _texSample = _program->uniformLocation("texSample");

    initTextures();

    _geometryEngine.init(_program, _ctImages.size());
}

void GLviewer::fetchMatrices() {
    _cameraTransformation.rotate(_alpha, 0, 1, 0);
    _cameraTransformation.rotate(_beta, 1, 0, 0);

    _cameraPosition = _cameraTransformation * QVector3D(0, 0, _distance);
    _cameraUpDirection = _cameraTransformation * QVector3D(0, 1, 0);

    _pMatrix.setToIdentity();
    //_pMatrix.perspective(60.0, (float)width()/(float)height(), 0.1, 100.0);
    _pMatrix.ortho(-2, 2, -2, 2, 0.001, 1000);

    _mMatrix.setToIdentity();

    _vMatrix.setToIdentity();
    _vMatrix.lookAt(_cameraPosition, QVector3D(0, 0, 0), _cameraUpDirection);
  //  matrix.rotate(100.0f * screen()->refreshRate(), 0, 1, 0);

    _program->setUniformValue(_shaderMatrix, _pMatrix * _vMatrix * _mMatrix);
}

void GLviewer::render() {
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    _program->bind();

    fetchMatrices();

    _program->setUniformValue(_texSample, 0);

    //glBindTexture(GL_TEXTURE_2D, _textureCVGL);
    _textureCV.bind();

    _geometryEngine.drawModel(_program);

    _program->release();
}

void GLviewer::initTextures() {
    /*
    glGenTextures(1, &_textureCVGL);
    glBindTexture(GL_TEXTURE_2D, _textureCVGL);

    glActiveTexture(GL_TEXTURE0); */

    cv::Mat image(*(_ctImages[100]));
    cv::flip(image, image, 0);

    _textureCV.setSize(image.cols, image.rows);
    _textureCV.setFormat(QOpenGLTexture::RGBAFormat);
    _textureCV.allocateStorage();

    QOpenGLPixelTransferOptions pixelOptions;
    pixelOptions.setAlignment((image.step & 3) ? 1 : 4);
    pixelOptions.setRowLength(image.step / image.elemSize());

    _textureCV.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (void *) image.data, &pixelOptions);

    _textureCV.setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV.setMagnificationFilter(QOpenGLTexture::Linear);
    _textureCV.setWrapMode(QOpenGLTexture::ClampToBorder);

    _textureCV.generateMipMaps();
/*
    cv::Mat image(*(_ctImages[100]));
    cv::flip(image, image, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, (image.step & 3) ? 1 : 4);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, image.step / image.elemSize());

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.cols,
            image.rows, 0, GL_RED, GL_UNSIGNED_SHORT, image.ptr());

   glGenerateMipmap(GL_TEXTURE_2D);

/*
    _textureCV.setWrapMode(QOpenGLTexture::WrapMode::Repeat);
    _textureCV.bind();
    glBindTexture(GL_TEXTURE_2D, _textureCVGL);

    cv::namedWindow("WINDOW");
    cv::imshow("WINDOW", *(_ctImages[100]));
    cv::waitKey(0);*/
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

        render();
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

        render();
    }

    qDebug() << _distance;

    event->accept();
}
