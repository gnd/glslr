#version 130

out vec4 PixelColor;
uniform vec2 resolution;
uniform sampler2D sony;

// press 'c' to start/stop sony

void main(void) {
	vec2 p = vec2( gl_FragCoord.x / resolution.x, 1.0 - gl_FragCoord.y / resolution.y);
	PixelColor = texture(sony, p);
}
