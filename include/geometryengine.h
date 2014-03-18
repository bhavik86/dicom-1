#ifndef GEOMETRYENGINE_H
#define GEOMETRYENGINE_H

#include <QtGui/QOpenGLFunctions>
#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLBuffer>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

class GeometryEngine : protected QOpenGLFunctions {

public:
    GeometryEngine();
    ~GeometryEngine();

    void init(QOpenGLShaderProgram * program);
    void drawModel(QOpenGLShaderProgram * program);

private:
    QOpenGLBuffer _vboVert;
    QOpenGLBuffer _vboColor;
    QOpenGLBuffer _vboInd;

    int _vertexLocation;
    int _texcoordLocation;

    GLuint _vboIds[2];

    void initGeometry();
};

#endif // GEOMETRYENGINE_H
