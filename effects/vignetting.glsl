/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform sampler2D prev_layer;

#define VIG_REDUCTION_POWER 1.65
#define VIG_BOOST 1.2

void main(void)
{
    vec2 uv = (gl_FragCoord.xy + vec2(0.5, 0.5)) / resolution.xy;
    vec2 center = resolution * 0.5;
    vec4 col;
    float vig;
    col = texture2D(prev_layer, uv);
    vig = distance(center, gl_FragCoord.xy) / resolution.x;
    vig = VIG_BOOST - vig * VIG_REDUCTION_POWER;
    gl_FragColor = col * vec4(vig, vig, vig, 1.0);
}
