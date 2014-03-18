#version 130
in highp vec4 qt_Vertex;
in highp vec4 qt_Color;
in highp vec4 qt_MultiTexCoord0;

uniform highp mat4 qt_ModelViewProjectionMatrix;

out highp vec4 qt_TexCoord0;
out highp vec4 aColor;

void main(void) {
    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
    qt_TexCoord0 = qt_MultiTexCoord0;
    aColor = qt_Color;
}
