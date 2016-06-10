/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform sampler2D prev_layer;
uniform vec2 mouse;

#define SCALE_FACTOR 10.0

void main(void)
{
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    gl_FragColor = texture2D(prev_layer, uv * mouse * SCALE_FACTOR);
}

