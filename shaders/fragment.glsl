#version 130
in highp vec4 fragTex;
in highp vec4 aColor;

uniform float rBottom;
uniform float rTop;
uniform sampler3D texSample;

void main(void) {
    vec4 colorTex = texture(texSample, fragTex.stp);

    gl_FragColor.rgba = colorTex.rrrr;

    if (gl_FragColor.r < rBottom || gl_FragColor.r > rTop) {
        discard;
    }
}
