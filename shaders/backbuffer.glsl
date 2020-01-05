#version 330

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
uniform float rand;
uniform sampler2D backbuffer;

void main(void) {
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    float c = distance(uv, mouse) > 0.12 ? 1.0 : 0.0;
    vec4 col = vec4(c, c, c, 1.0);
    gl_FragColor = mix(texture2D(backbuffer, uv), col, 0.05);
}
