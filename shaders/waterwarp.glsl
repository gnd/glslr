#version 130

// from: http://glsl.heroku.com/e#1157.0
// modified(light-weighted) for raspberry-pi

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

// An attempt at Quilez's warping (domain distortions):
// http://iquilezles.org/www/articles/warp/warp.htm
//
// Not as good as his, but still interesting.
// @SyntopiaDK, 2012



float rand(vec2 co){
	// implementation found at: lumina.sourceforge.net/Tutorials/Noise.html
	float k = sin(dot(co.xy ,vec2(12.9898,78.233)));
	return fract(k + k);
}

float noise2f( in vec2 p )
{
	vec2 ip = vec2(floor(p));
	vec2 u = fract(p);
	// http://www.iquilezles.org/www/articles/morenoise/morenoise.htm
	u = u*u;
	//u = u*u*u*((6.0*u-15.0)*u+10.0);

	float res = mix(
					mix(rand(ip), rand(ip+vec2(1.0,0.0)), u.x),
					mix(rand(ip+vec2(0.0,1.0)), rand(ip+vec2(1.0,1.0)), u.x),
					u.y);

	return res - 0.25;
	//return 2.0* (res-0.5);
}

float fbm(vec2 c) {
	float f = 0.0;
	f += noise2f(c);
	c += c;
	f += noise2f(c) * 0.5;
	c += c + c + c;
	return f + noise2f(c) * 0.3;
}

float pattern(  vec2 p, out vec2 r )
{
	//q.y = fbm( p );
	//q.x = q.y;
	r.x = fbm(p + vec2(1.7,9.2)+0.25*time);
	r.y = fbm(p + vec2(8.3,2.8)+0.326*time);
	return fbm(p +r);
}

const vec3 color1 = vec3(0.101961,0.619608,0.666667);
const vec3 color2 = vec3(0.666667,0.666667,0.698039);
//const vec3 color3 = vec3(0,0,0.164706);
const vec3 color4 = vec3(0.666667,1,1);
void main() {
	vec2 r;
	float f = pattern(gl_FragCoord.xy/ resolution.xy*10.0,r);
	vec3 col = mix(color1,color2,clamp(f*4.0,0.0,1.0));
	col = mix(color2,color4,clamp(r.x,0.0,1.0));
	gl_FragColor =  vec4((0.2*f*f+0.6*f)*col,1.0);
}
