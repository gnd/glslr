#version 330 

/* original source from http://glsl.heroku.com/e#5014.0 */
/* modified(light-weighted) for Raspberry Pi */

#ifdef GL_ES
precision mediump float;
#endif

uniform vec2 resolution;
uniform float time;

//Object A (tunnel)
float o(vec3 q)
{
	return cos(q.x) + cos(q.y) + cos(q.z) + cos(q.y*20.)*.05;
}


//Scene
float ox(vec3 q)
{
 return 0.0;
}

//Get Normal
vec3 gn(vec3 q)
{
 vec3 f=vec3(.01,0,0);
 return normalize(vec3(o(q+f.xyy),o(q+f.yxy),o(q+f.yyx)));
}

//MainLoop
void main(void)
{
 vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
 p.x *= resolution.x/resolution.y;

 vec3 org=vec3(sin(time)*0.5,0.25,time*2.0),dir=normalize(vec3(p.x*1.6,p.y,1.0)), q;
 float d;

 //First raymarching
  q=org+o(q)*dir;
  d=o(q);
  q+=d*dir;
  d=o(q);
  q+=d*dir;
  d=o(q);
  q+=d*dir;
  d=o(q);
  q+=d*dir;
  d=o(q);
  /*
	q+=d*dir;
	d=o(q);
    q+=d*dir;
	d=o(q);
 */
  q+=d*dir;
  q+=o(q)*dir+o(q)*dir;
 float f=length(q-org)*0.01;

 vec4 c;
 c= dot(gn(q), vec3(.1,.1,.0)) +
	 vec4(0.0, 0.0, 0.0, 1.0);

 //Final Color
 vec4 fcolor = ((c+vec4(f))+(1.-min(q.y+1.9,1.))*vec4(1.,.8,.7,1.));
 gl_FragColor=vec4(fcolor.xyz,1.0);
}
