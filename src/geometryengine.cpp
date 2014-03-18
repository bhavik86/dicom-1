#include "geometryengine.h"

typedef struct _VertexData {
    QVector3D position;
    QVector2D texCoord;
}VertexData;

GeometryEngine::GeometryEngine() :
    _vboVert(QOpenGLBuffer::VertexBuffer),
    _vboInd(QOpenGLBuffer::IndexBuffer) {

}

GeometryEngine::~GeometryEngine() {
    _vboVert.destroy();
    _vboInd.destroy();
}

void GeometryEngine::init() {
    initializeOpenGLFunctions();

    initGeometry();
}

void GeometryEngine::initGeometry() {
    _vboVert.create();
    _vboVert.setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);

    VertexData vertices[] = {
        {QVector3D(-1.0, -1.0,  1.0), QVector2D(0.0, 0.0)},  // v0
        {QVector3D( 1.0, -1.0,  1.0), QVector2D(0.33, 0.0)}, // v1
        {QVector3D(-1.0,  1.0,  1.0), QVector2D(0.0, 0.5)},  // v2
    };

    _vboVert.bind();
    _vboVert.allocate(&vertices, 3 * sizeof(VertexData));

    _vboInd.create();
    _vboInd.setUsagePattern(QOpenGLBuffer::UsagePattern::StaticDraw);

    GLushort indices[] = {
         2,  1,  0
    };

    _vboInd.bind();
    _vboInd.allocate(&indices, 3 * sizeof(GLushort));
}

void GeometryEngine::drawModel(QOpenGLShaderProgram * program) {
    //glBindBuffer(GL_ARRAY_BUFFER, _vboIds[0]);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _vboIds[1]);

    int offset = 0;

    int vertexLocation = program->attributeLocation("qt_Vertex");
    program->enableAttributeArray(vertexLocation);
    program->setAttributeBuffer(vertexLocation, GL_FLOAT, 0, 3, sizeof(VertexData));
    //glVertexAttribPointer(vertexLocation, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offset);

    offset += sizeof(QVector3D);

    int texcoordLocation = program->attributeLocation("qt_MultiTexCoord0");
    program->enableAttributeArray(texcoordLocation);
    program->setAttributeBuffer(texcoordLocation, GL_FLOAT, offset, 2, sizeof(VertexData));
    //glVertexAttribPointer(texcoordLocation, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offset);

    //_vboVert->bind();
    glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_BYTE, 0);

    qDebug() << program->log();
}
