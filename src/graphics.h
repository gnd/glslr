#include "base.h"
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#ifdef OSX
    #define GL_SILENCE_DEPRECATION
    #define GL_GLEXT_PROTOTYPES
    #define GLFW_INCLUDE_GLCOREARB
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
#endif

#ifdef LINUX
    #define GL_GLEXT_PROTOTYPES
    #include <GLFW/glfw3.h>
    #include "v4l2.h"
#endif

typedef struct {
	int numer;
	int denom;
} Scaling;

enum {
    MAX_RENDER_LAYER = 8,
    MAX_TEXTURES = 2
};

typedef struct JpegDec_s {
        unsigned long x;
        unsigned long y;
        unsigned short int bpp;
        unsigned char* data;
        unsigned long size;
        int channels;
} JpegDec_t;

extern pthread_mutex_t video_mutex;

typedef enum {
    Graphics_LAYOUT_PRIMARY_FULLSCREEN,
    Graphics_LAYOUT_PRIMARY_RESOLUTION,
    Graphics_LAYOUT_SECONDARY_RESOLUTION,
    Graphics_LAYOUT_SECONDARY_FULLSCREEN
} Graphics_LAYOUT;

typedef enum Graphics_PIXELFORMAT_ {
    Graphics_PIXELFORMAT_RGB888,
    Graphics_PIXELFORMAT_RGBA8888,
    Graphics_PIXELFORMAT_RGB565,
    Graphics_PIXELFORMAT_RGBA5551,
    Graphics_PIXELFORMAT_RGBA4444,
    Graphics_PIXELFORMAT_ENUMS
} Graphics_PIXELFORMAT;

typedef enum {
    Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR,
    Graphics_INTERPOLATION_MODE_BILINEAR,
    Graphics_INTERPOLATION_MODE_ENUMS
} Graphics_INTERPOLATION_MODE;

typedef enum {
    Graphics_WRAP_MODE_CLAMP_TO_EDGE,
    Graphics_WRAP_MODE_REPEAT,
    Graphics_WRAP_MODE_MIRRORED_REPEAT,
    Graphics_WRAP_MODE_ENUMS
} Graphics_WRAP_MODE;

// net_input values
typedef struct linked_val {
	float val;
	struct linked_val *next;
} netin_val;

// net_input addresses
typedef struct linked_addr {
	GLuint addr;
	struct linked_addr *next;
} netin_addr;

typedef struct displaydata {
  int window_id; /* id of the display window  */
  int window_width;  /* in pixels  */
  int window_height; /* in pixels  */
  int texture_width; /* may be window_width or next larger power of two  */
  int texture_height;/* may be window_height or next larger power of two  */
  int bytes_per_pixel; /* of the texture   */
  int internal_format; /* of the texture data  */
  int pixelformat;/* of the texture pixels  */
} Displaydata_t;

typedef struct RenderLayer_ {
	GLuint fragment_shader;
	GLuint program;
	GLuint texture_object;
	GLuint texture_unit;
	GLuint framebuffer;
	struct {
		GLuint vertex_coord;
		netin_addr *net_input_addr;
		GLuint mouse;
		GLuint time;
		GLuint resolution;
		GLuint backbuffer;
        GLuint video;
        GLuint sony;
		GLuint rand;
		GLuint prev_layer;
		GLuint prev_layer_resolution;
        GLuint lines_before_include;
        GLuint lines_included;
	} attr;
	void *auxptr;
} RenderLayer;

typedef struct Graphics_ {
	GLFWwindow* window;
	struct {
		int x;
		int y;
		int z;
		int w;
	} viewport;
	Graphics_LAYOUT layout;
	GLuint array_buffer_fullscene_quad;
	GLuint vertex_shader;
	Graphics_WRAP_MODE texture_wrap_mode;
	Graphics_INTERPOLATION_MODE texture_interpolation_mode;
	Graphics_PIXELFORMAT texture_pixel_format;
	RenderLayer render_layer[MAX_RENDER_LAYER];
	int num_render_layer;
    Displaydata_t displaydata;
	int num_static_image;
	int enable_backbuffer;
    int enable_video;
    int enable_sony;
	int net_params;
    GLuint textures[2];
	GLuint backbuffer_texture_object;
	GLuint backbuffer_texture_unit;
    GLuint video_texture_object;
    GLuint video_texture_unit;
    GLuint sony_texture_object;
    GLuint sony_texture_unit;
	Scaling window_scaling;
	Scaling primary_framebuffer;
} Graphics;

Graphics *Graphics_Create(Graphics_LAYOUT layout, int scaling_numer, int scaling_denom);

#ifdef LINUX
    void Graphics_InitDisplayData(Graphics *g, Sourceparams_t * sourceparams);
    void Graphics_Render(Graphics *g, Sourceparams_t * sourceparams, JpegDec_t* jpeg_dec);
#endif

#ifdef OSX
    void Graphics_InitDisplayData(Graphics *g);
    void Graphics_Render(Graphics *g, JpegDec_t* jpeg_dec);
#endif

void Graphics_HostInitialize(void);
void Graphics_HostDeinitialize(void);
void *RenderLayer_GetAux(RenderLayer *layer);
void Graphics_Delete(Graphics *g);
static void DeterminePixelFormat(Graphics_PIXELFORMAT pixel_format, GLint *out_internal_format, GLenum *out_format, GLenum *out_type);
int RenderLayer_UpdateShaderSource(RenderLayer *layer, const char *source, OPTIONAL int source_length);
int Graphics_AppendRenderLayer(Graphics *g, const char *source, int lines_before, int lines_included, OPTIONAL int source_length, OPTIONAL void *auxptr);
void Graphics_SetOffscreenPixelFormat(Graphics *g, Graphics_PIXELFORMAT pixel_format);
void Graphics_SetOffscreenInterpolationMode(Graphics *g, Graphics_INTERPOLATION_MODE interpolation_mode);
void Graphics_SetOffscreenWrapMode(Graphics *g, Graphics_WRAP_MODE wrap_mode);
int Graphics_ApplyOffscreenChange(Graphics *g);
void Graphics_SetWindowScaling(Graphics *g, int numer, int denom);
int Graphics_ApplyWindowScalingChange(Graphics *g);
void Graphics_SetupViewport(Graphics *g);
void Graphics_SetLayout(Graphics *g, Graphics_LAYOUT layout, int width, int height);
void Graphics_setWindowSize(int _width, int _height);
int Graphics_AllocateOffscreen(Graphics *g);
void Graphics_DeallocateOffscreen(Graphics *g);
RenderLayer *Graphics_GetRenderLayer(Graphics *g, int layer_index);
int Graphics_BuildRenderLayer(Graphics *g, int layer_index);
void Graphics_SetUniforms(Graphics *g, double t, netin_val *net_input_val, double mouse_x, double mouse_y, double randx, double randy);
void Graphics_SetBackbuffer(Graphics *g, int enable);
void Graphics_SetVideo(Graphics *g, int enable);
void Graphics_SetSony(Graphics *g, int enable);
void Graphics_SetNetParams(Graphics *g, int params);
Graphics_LAYOUT Graphics_GetCurrentLayout(Graphics *g);
Graphics_LAYOUT Graphics_GetLayout(Graphics_LAYOUT layout, int forward);
void Graphics_GetWindowSize(Graphics *g, int *out_width, int *out_height);
void Graphics_GetSourceSize(Graphics *g, int *out_width, int *out_height);
void Graphics_getWindowWidth(Graphics *g, int *width);
void Graphics_getWindowHeight(Graphics *g, int *height);
