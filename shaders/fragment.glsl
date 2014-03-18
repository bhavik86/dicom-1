#version 130
in highp vec4 qt_TexCoord0;
in highp vec4 aColor;

uniform sampler2D qt_Texture0;

void main(void) {
    gl_FragColor = texture2D(qt_Texture0, qt_TexCoord0.st);
}
