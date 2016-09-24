# glslr
**glslr** is a ultra-lightweight GLSL livecoding framework for x86 architecture.

**glslr** watches for changes in the input GLSL file and renders them to screen.

**glslr** is able to receive control data through a network socket which is then fed to the shader in the form of uniform variables.

**glslr** enables GLSL code-reuse across projects with its #include support.

##Installation:

On Ubuntu its enough to install libgles2-mesa-dev and libglfw3-dev:
```
apt-get install git libgles2-mesa-dev libglfw3-dev
git clone https://github.com/k-o-l-e-k-t-i-v/glslr
cd glslr
make
```

##Usage:
```
glslr example.glsl
```
##Usage with network:
```
glslr --net --port [port] --params N example.glsl
```

The parameters must be sent via UDP to the [port] in the form: 
*"param0 param1 param2 ... paramN;"*.
A reference is to use PureData's object [netsend].
Currently only integer and float variables are supported.

## Using parameters:

**glslr** makes the parameters available in GLSL as uniform float type. The parameters can be accessed from the code, where N is the number of parameters specified on the command line *(currently max 99)*, like this:
```
uniform float m0;
..
uniform float mN;
```

## Using include:

**glslr** provides support to include GLSL files from within the shader. Lines containing the string //#include file will be replaced with the contents of the file:


function.glsl:
```
color = vec3(1.,0.,0.);
```

project.glsl:
```
void main(void) {
	vec3 c=vec3(0.,1.,0.);

	//#include function.glsl  
    // ^^^ this line will be replaced with contents of the function.glsl file
	
	gl_FragColor = vec4(c, 1.0);
}
```


**glslr** is a fork of pijockey-sound (https://github.com/k-o-l-e-k-t-i-v/pijockey-sound), a GLSL livecoding framework for RaspberryPi, an extension of the original sourcecode of PiJockey by sharrow.
**glslr** was ported to x86, using GLFW.

