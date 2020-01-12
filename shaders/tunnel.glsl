#version 330
out vec4 PixelColor;
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

void main( void ) {
	 vec2 p = -1.0 + 2.0 * gl_FragCoord.xy / resolution.xy;
     vec2 uv;
     float r = sqrt( dot(p,p) );
     float a = atan(p.y,p.x) + 0.9*sin(0.5*r-0.5*time*0.1);
	 float s = 0.5 + 0.5*cos(7.0*a);

     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);
     s = smoothstep(0.0,1.0,s);

     uv.x = time*0.18 + 1.0/( r + .2*s);
	 uv.y = 1.0*a/10.1416;

     float w = (0.5 + 0.5*s)*r*r;
     float ao = 0.5 + 0.5*cos(42.0*a);//amp up the ao-g
     ao = smoothstep(0.0,0.4,ao)-smoothstep(0.4,0.7,ao);
     ao = 1.0-0.5*ao*r;

	 //faux shaded texture-gt
	 float px = gl_FragCoord.x/resolution.x;
	 float py = gl_FragCoord.y/resolution.y;
	 float x = mod(uv.x*resolution.x,resolution.x/3.0);
	 float y = mod(uv.y*resolution.y+(resolution.y/2.),resolution.y/3.5);
	 float v =  (x / y) - 0.7;
	 PixelColor = vec4(vec3(.9-v,.9-v,1.-v)*w*ao,1.0);
}
