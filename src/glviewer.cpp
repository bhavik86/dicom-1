#include <QtGui/QColor>
#include <QtGui/QScreen>

#include "glviewer.h"

GLviewer::GLviewer() :
    _program(0) {

}

GLviewer::~GLviewer() {
    delete _textureCV;
}

void GLviewer::loadModel(const std::vector<cv::Mat *> & ctImages) {
    _ctImages = ctImages;

    initTextures();

    render();
}

void GLviewer::initialize() {
    _program = new QOpenGLShaderProgram(this);
    _program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/vertex.glsl");
    _program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":shaders/fragment.glsl");
    _program->link();

    _shaderMatrix = _program->uniformLocation("qt_ModelViewProjectionMatrix");

   // glEnable(GL_DEPTH_TEST);
  //  glDisable(GL_CULL_FACE);
//    glShadeModel(GL_SMOOTH);

    _geometryEngine.init(_program);
}

void GLviewer::render() {
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);

    glClear(GL_COLOR_BUFFER_BIT);

    glClearColor(1.0f, 0.0f, 1.0f, 1.0f);

    _program->bind();

    QMatrix4x4 matrix;
    matrix.perspective(60, 4.0/3.0, 0.1, 100.0);
    matrix.translate(0, 0, -10);
  //  matrix.rotate(100.0f * screen()->refreshRate(), 0, 1, 0);

    _program->setUniformValue(_shaderMatrix, matrix);

    _geometryEngine.drawModel(_program);

    _program->release();

    qDebug() << _program->log();
}

void GLviewer::initTextures() {
    glEnable(GL_TEXTURE_2D);
/*
    std::cout << glGetError() << std::endl;

    glGenTextures(1, &_textureCV);
    glBindTexture(GL_TEXTURE_2D, _textureCV);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
*/
    _textureCV = new QOpenGLTexture(QImage(":shaders/image1.jpg"));
    _textureCV->setMinificationFilter(QOpenGLTexture::LinearMipMapNearest);
    _textureCV->setMagnificationFilter(QOpenGLTexture::Linear);
/*
    std::cout << glGetError() << std::endl;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, _ctImages[0]->cols,
            _ctImages[0]->rows, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, _ctImages[0]->data);

    std::cout << glGetError() << std::endl;
*/
    _textureCV->setWrapMode(QOpenGLTexture::WrapMode::Repeat);
    _textureCV->bind();
  //  glBindTexture(GL_TEXTURE_2D, _textureCV);
}
