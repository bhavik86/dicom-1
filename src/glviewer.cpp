#include <QtGui/QColor>
#include <QtGui/QScreen>

#include "glviewer.h"

#include "opencv2/highgui/highgui.hpp"

GLviewer::GLviewer(const std::vector<cv::Mat *> & ctImages) :
    _program(0),
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

    _shaderMatrix = _program->uniformLocation("qt_ModelViewProjectionMatrix");

    initTextures();

    _geometryEngine.init(_program);
}

void GLviewer::render() {
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    _program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60, width()/(double)height(), 0.1, 100.0);
    matrix.translate(0, 0, -10);
  //  matrix.rotate(100.0f * screen()->refreshRate(), 0, 1, 0);

    _program->setUniformValue(_shaderMatrix, matrix);

    _geometryEngine.drawModel(_program);

    _program->release();

    qDebug() << _program->log();
}

void GLviewer::initTextures() {/*
    glEnable(GL_TEXTURE_2D);

    qDebug() << glGetError();

    glGenTextures(1, &_textureCVGL);
    glBindTexture(GL_TEXTURE_2D, _textureCVGL);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);*/
/*
    _textureCV = new QOpenGLTexture(QImage(":shaders/image1.jpg").mirrored());
    _textureCV->setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV->setMagnificationFilter(QOpenGLTexture::Linear);
*/

    _textureCV.setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV.setMagnificationFilter(QOpenGLTexture::Linear);
    _textureCV.setSize(_ctImages[100]->cols, _ctImages[100]->rows);
    _textureCV.setFormat(QOpenGLTexture::LuminanceFormat);
    _textureCV.allocateStorage();

    _textureCV.setData(QOpenGLTexture::PixelFormat::Luminance, QOpenGLTexture::PixelType::UInt16, (void *) _ctImages[100]->data);

    qDebug() << glGetError();

    qDebug() << _textureCV.height();
  /*  glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _ctImages[100]->cols,
            _ctImages[100]->rows, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, _ctImages[100]->data);

    qDebug() << glGetError();
*/
    _textureCV.setWrapMode(QOpenGLTexture::WrapMode::Repeat);
    _textureCV.bind();
    //glBindTexture(GL_TEXTURE_2D, _textureCVGL);
/*
    cv::namedWindow("WINDOW");
    cv::imshow("WINDOW", *(_ctImages[100]));
    cv::waitKey(0);*/
}
