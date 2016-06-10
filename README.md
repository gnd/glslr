# glslr
glslr is a GLSL livecoding framework for x86 architecture.
glslr watches for changes in the input GLSL file and renders them to screen.
glslr is able to receive control data through a network socket which is then fed to the shader in the form of uniform variables.

Usage:
glslr example.glsl

Usage with network:
glslr --net --port PORT --params NUMBER example.glsl

The paramaters are sent as UDP to the PORT in the form: 
"param0 param1 param2 ... paramN;" (eg. by PureData using [netsend])

glslr makes them available in GLSL as 
uniform float m0;
 .. 
uniform float mN;


glslr is a fork of pijockey-sound (https://github.com/k-o-l-e-k-t-i-v/pijockey-sound), a glsl livecoding framework for RaspberryPi.
glslr was ported to x86, using GLFW.

