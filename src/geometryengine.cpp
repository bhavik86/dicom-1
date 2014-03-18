#include "geometryengine.h"

typedef struct _VertexData {
    QVector3D position;
    QVector3D color;
    QVector2D texCoord;
}VertexData;

GeometryEngine::GeometryEngine() :
    _vboVert(QOpenGLBuffer::VertexBuffer),
    _vboColor(QOpenGLBuffer::VertexBuffer),
    _vboInd(QOpenGLBuffer::IndexBuffer) {

}

GeometryEngine::~GeometryEngine() {
    _vboVert.destroy();
    _vboInd.destroy();
}

void GeometryEngine::init(QOpenGLShaderProgram * program) {
    initializeOpenGLFunctions();

    _vertexLocation = program->attributeLocation("qt_Vertex");
    _texcoordLocation = program->attributeLocation("qt_MultiTexCoord0");

    initGeometry();
}

void GeometryEngine::initGeometry() {
    _vboVert.create();
    _vboVert.setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);

    _vboColor.create();
    _vboColor.setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);

    VertexData vertices[] = {
        {QVector3D(-1.0, -1.0,  1.0), QVector3D(1.0, 0.0, 0.0), QVector2D(0.0, 0.0)},  // v0
        {QVector3D( 1.0, -1.0,  1.0), QVector3D(1.0, 0.0, 1.0), QVector2D(0.33, 0.0)}, // v1
        {QVector3D(-1.0,  1.0,  1.0), QVector3D(1.0, 0.0, 0.0), QVector2D(0.0, 0.5)},  // v2
    };

    _vboVert.bind();
    _vboVert.allocate(&vertices, 3 * sizeof(VertexData));

    _vboColor.bind();
    _vboColor.allocate(&vertices, 3 * sizeof(VertexData));

    _vboInd.create();
    _vboInd.setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);

    GLushort indices[] = {
         0,  1,  2
    };

    _vboInd.bind();
    _vboInd.allocate(&indices, 3 * sizeof(GLushort));
}

void GeometryEngine::drawModel(QOpenGLShaderProgram * program) {
    int offset = 0;

    program->enableAttributeArray(_vertexLocation);
    program->setAttributeBuffer(_vertexLocation, GL_FLOAT, offset, 3, sizeof(VertexData));

    offset += sizeof(QVector3D);

    program->enableAttributeArray("qt_Color");
    program->setAttributeBuffer("qt_Color", GL_FLOAT, offset, 3, sizeof(VertexData));

    offset += sizeof(QVector3D);

    program->enableAttributeArray(_texcoordLocation);
    program->setAttributeBuffer(_texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));

    _vboVert.bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    qDebug() << program->log();
}
