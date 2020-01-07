#version 130

/*
 * from http://glsl.heroku.com/e#5655.1
 */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 mouse;
uniform vec2 resolution;
uniform float time;
out vec4 PixelColor;

const int max_iteration = 32;

void main( void )
{
	vec2 c = (.1-mouse) * 2.5 + sin(time/3.);
	vec2 z = (gl_FragCoord.xy) / resolution * 3.0 - 1.5;

	float total = 0.;
	float m = 0.;
	for (int i = 0; i < max_iteration; ++i) {
		float pm = m;
		m = dot(z,z*2.);
		z = abs(z)/m + c;
		total += exp(-1./abs(m - pm));
	}

	total = total / 30.;
	PixelColor = vec4(total*1.5,total,total/sin(cos(time/10.)), 1.0);
}
