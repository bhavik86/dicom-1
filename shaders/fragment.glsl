#version 130
in highp vec4 fragTex;
in highp vec4 aColor;

uniform sampler2D texSample;

void main(void) {
    vec4 colorToSwizzle;

    colorToSwizzle = texture2D(texSample, fragTex.st);

    gl_FragColor.rgb = colorToSwizzle.rrr;
    gl_FragColor.a = 1.0;
}
