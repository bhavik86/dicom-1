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
    _textureCV3D(QOpenGLTexture::Target3D),
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

    _count = _ctImages.size();

    initTextures();

    _geometryEngine.init(_program, _count);
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

   // glBindTexture(GL_TEXTURE_2D, _textureCVGL);
    //_textureCV.bind();

    _geometryEngine.drawModel(_program);

    _program->release();
}

void GLviewer::initTextures() {
   /*
    cv::Mat image1(*(_ctImages[0]));
    cv::Mat image;
    cv::flip(image1, image1, 0);
    cv::cvtColor(image1, image, CV_GRAY2RGBA);

    _textureCV.setSize(image.cols, image.rows);
    _textureCV.setFormat(QOpenGLTexture::RGBA_DXT5);
    _textureCV.allocateStorage();

    QOpenGLPixelTransferOptions pixelOptions;
    pixelOptions.setAlignment((image.step & 3) ? 1 : 4);
    pixelOptions.setRowLength(image.step1());
/*
    //_textureCV.setCompressedData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (void *) image.data, &pixelOptions);
    _textureCV.setCompressedData(image.elemSize() * image.total(), (void *) image.data, 0);// &pixelOptions);

    _textureCV.setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV.setMagnificationFilter(QOpenGLTexture::Linear);
    _textureCV.setWrapMode(QOpenGLTexture::ClampToBorder);

    _textureCV.generateMipMaps();

*
    int byteSizeMat = _ctImages[0]->elemSize() * _ctImages[0]->total();
    int byteSizeAll = byteSizeMat * _count;

    qDebug() << byteSizeAll;

    uchar * data = new uchar[byteSizeAll];

    for (int i = 0; i != _count; ++ i) {
        cv::flip(*(_ctImages[i]), image, 0);

        memcpy(data + byteSizeMat * i, image.data, byteSizeMat);
    }

    qDebug() << image.cols * image.rows * _ctImages.size();

    _textureCV3D.setSize(image.cols, image.rows, _count);//_ctImages.size());
    _textureCV3D.setFormat(QOpenGLTexture::RGBAFormat);
    _textureCV3D.allocateStorage();

    _textureCV3D.setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16, (void *) data, &pixelOptions);

    _textureCV3D.setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV3D.setMagnificationFilter(QOpenGLTexture::Linear);
    _textureCV3D.setWrapMode(QOpenGLTexture::ClampToBorder);

    _textureCV3D.generateMipMaps();


*/
   glGenTextures(1, &_textureCVGL);
   glBindTexture(GL_TEXTURE_3D, _textureCVGL);

   glActiveTexture(GL_TEXTURE0);

   int byteSizeMat = _ctImages[0]->elemSize() * _ctImages[0]->total();
   int byteSizeAll = byteSizeMat * _count;

   cv::Mat image;

   uchar * data = new uchar[byteSizeAll];

   for (int i = 0; i != _count; ++ i) {
       cv::flip(*(_ctImages[i]), image, 0);

       memcpy(data + byteSizeMat * i, image.data, byteSizeMat);
   }


  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);

  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glPixelStorei(GL_UNPACK_ALIGNMENT, (image.step & 3) ? 1 : 4);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, image.step / image.elemSize());

  glTexImage3D(GL_TEXTURE_3D, 0, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, image.cols, image.rows, _count, 0, GL_RED, GL_UNSIGNED_SHORT, data);

  glGenerateMipmap(GL_TEXTURE_3D);

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
