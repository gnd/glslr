/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform vec2 mouse;
uniform sampler2D prev_layer;

uniform float time;
 
void main(void)
{
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    vec4 col = texture2D(prev_layer, uv);
    vec4 col2, col3;
    vec2 slide = vec2(0.008, 0.0);
    col2 = texture2D(prev_layer, uv-slide);
    col3 = texture2D(prev_layer, uv+slide);
    col = vec4(col2.r, col.g, col3.b, col.a);
    gl_FragColor = col;
}
