#ifndef INCLUDED_GRAPHICS_H
#define INCLUDED_GRAPHICS_H

#include <stddef.h>
#include "base.h"

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include "v4l2.h"

typedef enum {
    Graphics_LAYOUT_PRIMARY_FULLSCREEN,
    Graphics_LAYOUT_PRIMARY_RESOLUTION,
    Graphics_LAYOUT_SECONDARY_RESOLUTION,
    Graphics_LAYOUT_SECONDARY_FULLSCREEN
} Graphics_LAYOUT;

typedef enum {
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


typedef struct Graphics_ Graphics;
typedef struct RenderLayer_ RenderLayer;

void Graphics_HostInitialize(void);
void Graphics_HostDeinitialize(void);

void *RenderLayer_GetAux(RenderLayer *layer);
int RenderLayer_UpdateShaderSource(RenderLayer *layer,
                                   const char *source,
                                   OPTIONAL int source_length);


Graphics *Graphics_Create(Graphics_LAYOUT layout,
                          int scaling_numer, int scaling_denom);
                          
void Graphics_Delete(Graphics *g);
void Graphics_InitDisplayData(Graphics *g, Sourceparams_t * sourceparams);

int Graphics_AppendRenderLayer(Graphics *g,
                               const char *source,
                               int lines_before,
                               int lines_included,
                               OPTIONAL int source_length,
                               OPTIONAL void *auxptr);

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
void Graphics_SetUniforms(Graphics *g, double t,
                          netin_val *net_input_val,
                          double mouse_x, double mouse_y,
                          double randx, double randy);
void Graphics_Render(Graphics *g, Sourceparams_t * sourceparams);
void Graphics_SetBackbuffer(Graphics *g, int enable);
void Graphics_SetVideo(Graphics *g, int enable);
void Graphics_SetNetParams(Graphics *g, int params);
Graphics_LAYOUT Graphics_GetCurrentLayout(Graphics *g);
Graphics_LAYOUT Graphics_GetLayout(Graphics_LAYOUT layout, int forward);
void Graphics_GetWindowSize(Graphics *g, int *out_width, int *out_height);
void Graphics_GetSourceSize(Graphics *g, int *out_width, int *out_height);
void Graphics_getWindowWidth(Graphics *g, int *width);
void Graphics_getWindowHeight(Graphics *g, int *height);

#endif