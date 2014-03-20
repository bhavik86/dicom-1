#include "geometryengine.h"

typedef struct _VertexData {
    QVector3D position;
    QVector3D texCoord;
}VertexData;

GeometryEngine::GeometryEngine() :
    _vboVert(QOpenGLBuffer::VertexBuffer),
    _vboInd(QOpenGLBuffer::IndexBuffer) {

}

GeometryEngine::~GeometryEngine() {
    _vboVert.destroy();
    _vboInd.destroy();
}

void GeometryEngine::init(QOpenGLShaderProgram * program, const int & count) {
    initializeOpenGLFunctions();

    _shaderVertex = program->attributeLocation("vertex");
    _shaderTex = program->attributeLocation("tex");

    initGeometry(count);
}

void GeometryEngine::initGeometry(const int & count) {
    _vboVert.create();
    _vboVert.setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);

    int vertexCount = 4 * count;
    int indexCount = 6 * count;

    VertexData vertices[vertexCount];
    GLushort indices[indexCount];

    float step = 2.0 / (float) count;
    float stepTexture = 1.0 / (float) count;

    float zCurrent = -1.0;
    float zCurrentTexture = 0.0;

    int currentVert = 0;
    int currentIndex = 0;

    for (int i = 0; i != count; ++ i) {
        vertices[currentVert ++] = {QVector3D(-1.0, -1.0,  zCurrent), QVector3D(0.0, 0.0, zCurrentTexture)};
        vertices[currentVert ++] = {QVector3D(-1.0, 1.0,  zCurrent), QVector3D(0.0, 1.0, zCurrentTexture)};
        vertices[currentVert ++] = {QVector3D(1.0, 1.0,  zCurrent), QVector3D(1.0, 1.0, zCurrentTexture)};
        vertices[currentVert ++] = {QVector3D(1.0, -1.0,  zCurrent), QVector3D(1.0, 0.0, zCurrentTexture)};

        indices[currentIndex ++] = 4 * i;
        indices[currentIndex ++] = 4 * i + 1;
        indices[currentIndex ++] = 4 * i + 2;
        indices[currentIndex ++] = 4 * i;
        indices[currentIndex ++] = 4 * i + 2;
        indices[currentIndex ++] = 4 * i + 3;

        zCurrent += step;
        zCurrentTexture += stepTexture;
    };

    _vboVert.bind();
    _vboVert.allocate(&vertices, vertexCount * sizeof(VertexData));

    _vboInd.create();
    _vboInd.setUsagePattern(QOpenGLBuffer::UsagePattern::DynamicDraw);

    _vboInd.bind();
    _vboInd.allocate(&indices, indexCount * sizeof(GLushort));

    _indexCount = indexCount;
}

void GeometryEngine::drawModel(QOpenGLShaderProgram * program) {
    int offset = 0;

    program->enableAttributeArray(_shaderVertex);
    program->setAttributeBuffer(_shaderVertex, GL_FLOAT, offset, 3, sizeof(VertexData));

    offset += sizeof(QVector3D);

    program->enableAttributeArray(_shaderTex);
    program->setAttributeBuffer(_shaderTex, GL_FLOAT, offset, 3, sizeof(VertexData));

    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_SHORT, 0);

}
