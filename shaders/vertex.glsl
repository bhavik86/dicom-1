#version 330 core
in highp vec4 qt_Vertex;
in highp vec4 qt_MultiTexCoord0;

uniform highp mat4 qt_ModelViewProjectionMatrix;

out highp vec4 qt_TexCoord0;

void main(void) {
    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
    qt_TexCoord0 = qt_MultiTexCoord0;
}
