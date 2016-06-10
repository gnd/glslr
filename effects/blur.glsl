/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform vec2 mouse;
uniform sampler2D prev_layer;
uniform float time;

#define BLUR_POWER 8.0

void main(void)
{
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    vec2 center = resolution * 0.5;
    float vig;
    vec4 col1, col2, col3, col4;
    vig = distance(center, gl_FragCoord.xy) / resolution.x;
    vec2 pixel_size = 1.0 / (resolution + 0.5) * (vig * BLUR_POWER);
    col1 = texture2D(prev_layer, uv);
    col2 = texture2D(prev_layer, uv+vec2(0.0, pixel_size.y));
    col3 = texture2D(prev_layer, uv+vec2(pixel_size.x, 0.0));
    col4 = texture2D(prev_layer, uv+pixel_size);
    gl_FragColor = (col1 + col2 + col3 + col4) * 0.25;
}
