#version 130
in highp vec4 vertex;
in highp vec4 color;
in highp vec4 tex;

uniform highp mat4 mvpMatrix;

out highp vec4 fragTex;
out highp vec4 aColor;

void main(void) {
    gl_Position = mvpMatrix * vertex;
    fragTex = tex;
    aColor = color;
}
