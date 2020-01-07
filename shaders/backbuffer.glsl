#version 130

#ifdef GL_ES
precision mediump float;
#endif

// dont forget to turn on backbuffer

uniform float time;
uniform vec2 resolution;
uniform vec2 rand;
uniform sampler2D backbuffer;
out vec4 PixelColor;

void main(void) {
    vec2 uv = (gl_FragCoord.xy / resolution.xy);
    float c = distance(uv, vec2(sin(time)/2.5+.5, cos(time)/2.5+.5)) > 0.1 ? 1.0 : 0.0;
    vec4 col = vec4(c, c, c, 1.0);
    PixelColor = mix(texture2D(backbuffer, uv), col, 0.03);
}
