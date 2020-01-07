# glslr

## Features
- Ultra-lightweight GLSL livecoding framework for Linux and Mac OS X.
- Enables quick fragment shader development in your favorite editor.
- Changes in the fragment shader are compiled in realtime.
- Uniform variables can be filled with data received from a socket.
- Enables GLSL code-reuse across projects with support of #include.
- Has experimental Video4Linux support.
- Has support for livestreaming from a Sony AS30 ActionCam.

## Installation:

On Ubuntu:
```
apt-get install git libgles2-mesa-dev libglfw3-dev
git clone https://github.com/gnd/glslr
cd glslr
make
```
On Mac OSX:
```
(get CodeX, homebrew)
brew install glfw3 glew libjpeg
git clone https://github.com/gnd/glslr
cd glslr 
make
```
Please note that you can edit the src/makefile to enable/disable v4l2 support, simply by changing VIDEO=yes or VIDEO=no before compilation. V4l2 support is disabled when compiing on Mac OSX.

## Usage:

This will read the fragment shader from shaders/tunnel.glsl and render it in a 800x600 window on the primary monitor. Any changes you make to the file while **glslr** is running, will be automatically recompiled and sent to the GPU on save:
```
glslr shaders/tunnel.glsl
```

This will read the fragment shader from shaders/template.glsl and render it into a 1024x768 window on the primary monitor:
```
glslr --primary-res 1024x768 shaders/template.glsl
```

This will read the fragment shader and render it fullscreen on the secondary monitor (if any):
```
glslr --secondary-fs shaders/glass_sphere.glsl
```

For other options see bellow:
```
usage: glslr [options] <layer0.glsl> [layer1.glsl] [layer2.glsl] ...
options:
  window properties:
    --primary-fs                            create a fullscreen window on primary monitor
    --primary-res [WidthxHeight]            create a width x height window on primary monitor (default: 800x600)
    --secondary-fs                          create a fullscreen window on secondary monitor
    --secondary-res [WidthxHeight]          create a width x height window on secondary monitor
  offscreen format:
    --RGB888
    --RGBA8888 (default)
    --RGB565
    --RGBA4444
  interpolation mode:
    --nearestneighbor (default)
    --bilinear
  wrap mode:
    --wrap-clamp_to_edge
    --wrap-repeat (default)
    --wrap-mirror_repeat
  backbuffer:
    --backbuffer                            enable backbuffer (default:OFF)
  network:
    --net                                   enable network (default:OFF)
    --tcp                                   enable TCP (default:UDP)
    --port [port]                           listen on port (default:6666)
    --params [n]                            number of net input params (default:0)
  sony:
     --sony                                 enable Sony ActionCAM AS30 support
  video:
    --vdev [device number]                  v4l2 device number (default: 0 eg. /dev/video0)

```
## Usage with network:
```
glslr --net --port [port] --params N example.glsl
```

The parameters must be sent via UDP to the [port] in the form: *"param0 param1 param2 ... paramN;"*.  
A reference is to use PureData's object [netsend]. Currently only integer and float variables are supported.

**glslr** makes the received data available in the shader as uniform float. The data can be accessed if you declare pamaters like this in the shader:
```
uniform float m0;
..
uniform float mN;
```
You can use the parameters to carry input from a MIDI controller, sound-analysis or anything you like. The maximum count of parameters is 99 for now.

## Usage with video:

**glslr** makes it possible to use experimental live video input from a V4L device. Currently only YUV422 devices are supported. The image is raw YUV422 - conversion into RGBA is up to you in the shader.
```
glslr --vdev [video device number] example.glsl
```
Provide the video device number to be able to use it in the shader. Default is 0 which expands to /dev/video0.  
Eg. if you want to use /dev/video1 do:

```
glslr --vdev 1 shaders/test_video.glsl
````
In the shader you can access the video data by using uniform sampler2D variable called video. To start camera press 'v'.
```
#version 130

out vec4 PixelColor;
uniform vec2 resolution;
uniform sampler2D video;

// press 'v' to start video

void main(void) {
	vec2 p = vec2( gl_FragCoord.x / resolution.x, 1.0 - gl_FragCoord.y / resolution.y);
	PixelColor = texture(video, p);
}
```

## Using Sony ActionCam input:

**glslr** makes it possible to use live video input from a Sony AS30 ActionCam. 
```
glslr --sony shaders/test_sony.glsl
```

In the shader you can access the video data by using uniform sampler2D variable called sony:
```
#version 130

out vec4 PixelColor;
uniform vec2 resolution;
uniform sampler2D sony;

void main(void) {
	vec2 p = vec2( gl_FragCoord.x / resolution.x, 1.0 - gl_FragCoord.y / resolution.y);
	PixelColor = texture(sony, p);
}
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
	vec3 color = vec3(0.,1.,0.);

	//#include function.glsl  
    // ^^^ this line will be replaced with contents of the function.glsl file

	gl_FragColor = vec4(color, 1.0);
}
```
## 

**glslr** is a fork of pijockey-sound (https://github.com/gnd/pijockey-sound), a GLSL livecoding framework for RaspberryPi, an extension of the original sourcecode of **PiJockey by sharrow**.  
**glslr** was ported to x86, using GLFW. The Video4Linux capability was scrambled from **glutcam** by George Koharchik.  
**glslr** is being occasionally developed (when preparing for a new performance) by gnd ♥ gnd.sk.
