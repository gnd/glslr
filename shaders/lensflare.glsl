#version 330 

/* from http://glsl.heroku.com/e#5254.1 */

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

//space sun

#define BLADES 6.0
#define BIAS 0.1
#define SHARPNESS 3.0
#define COLOR 0.82, 0.65, 0.4
#define BG 0.34, 0.52, 0.76

void main( void ) {
	vec2 sunpos = (mouse - 0.5) / vec2(resolution.y/resolution.x,1.0);
	vec2 position = (( gl_FragCoord.xy / resolution.xy ) - vec2(0.5)) / vec2(resolution.y/resolution.x,1.0);
	vec3 color = vec3(0.);
	float blade = clamp(pow(sin(atan(position.y - sunpos.y, position.x - sunpos.x)*BLADES)+BIAS, SHARPNESS), 0.0, 1.0);
	color = mix(vec3(0.34, 0.5, 1.0), vec3(0.0, 0.5, 1.0), (position.y + 1.0) * 0.5);
	color += (vec3(0.95, 0.65, 0.30) * 1.0 / distance(sunpos, position) * 0.075) * 1.0 - distance(vec2(0.0), position);
	color += vec3(0.95, 0.45, 0.30) * min(1.0, blade *0.7) * (1.0 / distance(sunpos, position) * 0.075);
	gl_FragColor = vec4( color, 1.0 );
}
