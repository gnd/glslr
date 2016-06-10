/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform sampler2D prev_layer;
uniform sampler2D backbuffer;

void main(void)
{
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    gl_FragColor = texture2D(prev_layer, uv);
}
