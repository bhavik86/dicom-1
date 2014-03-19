#version 130
in highp vec4 fragTex;
in highp vec4 aColor;

uniform int count;
uniform sampler3D texSample;

void main(void) {
    vec4 colorToSwizzle = texture3D(texSample, fragTex.stp);

    gl_FragColor = colorToSwizzle;
    //gl_FragColor.rgba = colorToSwizzle.rrrr;
/*
    float step = 1.0 / count;
    float colorBlend = 0;

    vec3 color;

    color.st = fragTex.st;
    color.p = 0.0;

    for (float z = 0.0; z != 1.0; z += step) {
        color += vec3(0.0, 0.0, z);
        colorBlend += texture3D(texSample, color).r;
    }

    gl_FragColor.rgba = vec4(step * colorBlend);
*/
    if (gl_FragColor.r < 0.0001) {
        discard;
    }
}
