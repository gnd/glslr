#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#include "config.h"
#include "base.h"
#include "graphics.h"


/* TODO
* - make the windowed / fullscreen behaviour on keypress
*/


enum {
    MAX_RENDER_LAYER = 8,
    MAX_TEXTURES = 2
};

typedef struct {
	int numer;
	int denom;
} Scaling;

static GLFWwindow* window;

struct RenderLayer_ {
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
		GLuint rand;
		GLuint prev_layer;
		GLuint prev_layer_resolution;
        GLuint lines_before_include;
        GLuint lines_included;
	} attr;
	void *auxptr;
};


struct Graphics_ {
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
	int net_params;
    GLuint textures[2];
	GLuint backbuffer_texture_object;
	GLuint backbuffer_texture_unit;
    GLuint video_texture_object;
    GLuint video_texture_unit;
	Scaling window_scaling;
	Scaling primary_framebuffer; /* TODO */
};



static void DeterminePixelFormat(Graphics_PIXELFORMAT pixel_format,
                                 GLint *out_internal_format,
                                 GLenum *out_format, GLenum *out_type);

#ifdef NDEBUG
# define CHECK_GL()
#else
# define CHECK_GL() CheckGLError(__FILE__, __LINE__, __func__)
#endif

#ifndef NDEBUG
static void CheckGLError(const char *file, int line, const char *func)
{
	GLenum e;
	static const struct {
		const char *str;
		GLenum code;
	} tbl[] = {
		{ "GL_INVALID_ENUM", 0x0500 },
		{ "GL_INVALID_VALUE", 0x0501 },
		{ "GL_INVALID_OPERATION", 0x0502 },
		{ "GL_OUT_OF_MEMORY", 0x0505 }
	};

	e = glGetError();
	if (e != 0) {
		int i;
		printf("\nfrom %s(%d): function %s\n", file, line, func);
		for (i = 0; i < (int)ARRAY_SIZEOF(tbl); i++) {
			if (e == tbl[i].code) {
				printf("  OpenGL|ES raise: code 0x%04x (%s)\n", e, tbl[i].str);
				return;
			}
		}
		printf("  OpenGL|ES raise: code 0x%04x (?)\n", e);
	}
}
#endif

void handleError(const char *message, int _exitStatus)
{
	fprintf(stderr, message);
	exit(_exitStatus);
}

float fixDpiScale(GLFWwindow* _window)
{
	int window_width, window_height, framebuffer_width, framebuffer_height;
	glfwGetWindowSize(_window, &window_width, &window_height);
	glfwGetFramebufferSize(_window, &framebuffer_width, &framebuffer_height);
	return framebuffer_width/window_width;
}

void handleResize(GLFWwindow* _window, int _w, int _h)
{
	float dpiScale = fixDpiScale(_window);
	float inter_w = (float)_w * dpiScale;
	float inter_h = (float)_h * dpiScale;
	Graphics_setWindowSize((int)inter_w,(int)inter_h);
}

void Graphics_HostInitialize(void)
{
	if(!glfwInit()) {
		handleError("GLFW init failed", -1);
	}
}

int isGL()
{
	return !glfwWindowShouldClose(window);
}

void Graphics_HostDeinitialize(void)
{
}

static void PrintShaderLog(const char *message, GLuint shader, int before, int included)
{
    char *a, *b, *line_str;
    int line_num, in_include = 0;
	GLchar build_log[512];
    
    glGetShaderInfoLog(shader, sizeof(build_log), NULL, build_log);
    if (!before && !included) {
        printf("%s %d: %s\n", message, shader, build_log);
    } else { // compute the real line number in the layer source file
        a = strstr(build_log, "(");
        b = strstr(build_log, ")");
        line_num = strtol(a+1,NULL,10);
        if (line_num > (before + included)) {
            line_num = line_num - included + 1;
        } else {
            if (line_num > before) {
                line_num = line_num - before;
                in_include = 1;
            }
        }
        line_str = malloc((10 + strlen(b)) * sizeof(char));  //should not be bigger than 10 digits
        sprintf(line_str, "%d%s", line_num, b);
        strcpy(a+1, line_str);
        if (!in_include) {
            printf("%s %d: %s\n", message, shader, build_log);
        } else {
            printf("%s %d: %s %s\n", message, shader, "in include:", build_log);
        }
    }
}

static void PrintProgramLog(const char *message, GLuint program)
{
	GLchar build_log[512];
	glGetProgramInfoLog(program, sizeof(build_log), NULL, build_log);
}


static void Scaling_Apply(Scaling *sc, int *inout_width, int *inout_height)
{
	*inout_width = (*inout_width * sc->numer) / sc->denom;
	*inout_height = (*inout_height * sc->numer) / sc->denom;
}

/* unused
static int Scaling_IsOne(Scaling *sc)
{
	return (sc->numer == sc->denom) ? 1 : 0;
} */


/* RenderLayer */
static int RenderLayer_Construct(RenderLayer *layer,
                                int lines_before,
                                int lines_included,
                                 OPTIONAL void *auxptr)
{
	static const RenderLayer render_layer0 = { 0 };
	*layer = render_layer0;
	layer->auxptr = auxptr;
	layer->fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    layer->attr.lines_before_include = lines_before;
    layer->attr.lines_included = lines_included;
	return (layer->fragment_shader == 0) ? 1 : 0;
}

static void RenderLayer_Destruct(RenderLayer *layer)
{
	glDeleteProgram(layer->program);
	layer->program = 0;
	glDeleteShader(layer->fragment_shader);
	layer->fragment_shader = 0;
	assert(layer->texture_object == 0);
}

void *RenderLayer_GetAux(RenderLayer *layer)
{
	return layer->auxptr;
}

int RenderLayer_UpdateShaderSource(RenderLayer *layer,
                                   const char *source,
                                   OPTIONAL int source_length)
{
	glShaderSource(layer->fragment_shader, 1, &source, &source_length);
	if (glGetError() != 0) {
		return 1;
	}
	return 0;
}

static int RenderLayer_AllocateOffscreen(RenderLayer *layer,
        int is_final_layer, int tex_unit,
        int tex_width, int tex_height,
        Graphics_PIXELFORMAT pixel_format,
        Graphics_INTERPOLATION_MODE interpolation_mode,
        Graphics_WRAP_MODE wrap_mode)
{
	/* TODO: handle error */
	GLint internal_format;
	GLenum format;
	GLenum type;
	GLint interpolation;
	GLint wrap;

	CHECK_GL();

	DeterminePixelFormat(pixel_format, &internal_format, &format, &type);

	switch (interpolation_mode) {
	case Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR:
		interpolation = GL_NEAREST;
		break;
	case Graphics_INTERPOLATION_MODE_BILINEAR:
		interpolation = GL_LINEAR;
		break;
	default:
		assert(0);
		interpolation = GL_NEAREST;
		break;
	}

	switch (wrap_mode) {
	case Graphics_WRAP_MODE_CLAMP_TO_EDGE:
		wrap = GL_CLAMP_TO_EDGE;
		break;
	case Graphics_WRAP_MODE_REPEAT:
		wrap = GL_REPEAT;
		break;
	case Graphics_WRAP_MODE_MIRRORED_REPEAT:
		wrap = GL_MIRRORED_REPEAT;
		break;
	default:
		assert(0);
		wrap = GL_REPEAT;
		break;
	}

	if (is_final_layer) {
		/* use FRAMEBUFFER = 0 */
		layer->texture_unit = 0; /* not use */
		layer->texture_object = 0;
		layer->framebuffer = 0;
	} else {
		layer->texture_unit = tex_unit;
		glGenTextures(1, &layer->texture_object);
		glBindTexture(GL_TEXTURE_2D, layer->texture_object);
		glTexImage2D(GL_TEXTURE_2D,
		             0,             /* level */
		             internal_format,
		             tex_width, tex_height,
		             0,             /* border */
		             format,
		             type,
		             NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, interpolation);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, interpolation);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
		glBindTexture(GL_TEXTURE_2D, 0);

		glGenFramebuffers(1, &layer->framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, layer->framebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, layer->texture_object, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	CHECK_GL();
	return 0;
}

static void RenderLayer_DeallocateOffscreen(RenderLayer *layer)
{
	glBindTexture(GL_TEXTURE_2D, 0);
	if (layer->texture_object) {
		glDeleteTextures(1, &layer->texture_object);
		layer->texture_object = 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	if (layer->framebuffer) {
		glDeleteFramebuffers(1, &layer->framebuffer);
		layer->framebuffer = 0;
	}
}

static int RenderLayer_BuildProgram(RenderLayer *layer,
                                    GLuint vertex_shader,
                                    GLuint array_buffer_fullscene_quad,
                                    int net_params)
{
	GLint param;
	GLuint new_program;
	netin_addr *curr=NULL;
	netin_addr *next=NULL;
	int i;
	char name[5]; /* MAX 99 ! */

	CHECK_GL();
	glCompileShader(layer->fragment_shader);
    FILE *fp;
	glGetShaderiv(layer->fragment_shader, GL_COMPILE_STATUS, &param);
	if (param != GL_TRUE) {
		PrintShaderLog("fragment_shader", layer->fragment_shader, layer->attr.lines_before_include, layer->attr.lines_included);
		return 2;
	}

	new_program = glCreateProgram();
	glAttachShader(new_program, vertex_shader);
	glAttachShader(new_program, layer->fragment_shader);
	glLinkProgram(new_program);
	glGetProgramiv(new_program, GL_LINK_STATUS, &param);
	if (param != GL_TRUE) {
		PrintProgramLog("program", new_program);
		return 3;
	}

	glDeleteProgram(layer->program);
	layer->program = new_program;


	glUseProgram(layer->program);
	layer->attr.vertex_coord = glGetAttribLocation(layer->program, "vertex_coord");
	layer->attr.time = glGetUniformLocation(layer->program, "time");

	// free some memory before
	next = layer->attr.net_input_addr;
	while ((curr = next) != NULL) {
		next = next->next;
		free (curr);
	}

	// stores m_{net_params-1, 0} adresses
	i = net_params-1;
	while (i >= 0) {
		curr=malloc(sizeof(netin_addr));
		curr->next = next;
		sprintf(name, "m%d", i);
		curr->addr = glGetUniformLocation(layer->program, name);
		next = curr;
		i--;
	}
	layer->attr.net_input_addr = next;

	layer->attr.mouse = glGetUniformLocation(layer->program, "mouse");
	layer->attr.resolution = glGetUniformLocation(layer->program, "resolution");
	layer->attr.backbuffer = glGetUniformLocation(layer->program, "backbuffer");
    layer->attr.video = glGetUniformLocation(layer->program, "video");
	layer->attr.rand= glGetUniformLocation(layer->program, "rand");

	layer->attr.prev_layer = glGetUniformLocation(layer->program, "prev_layer");
	layer->attr.prev_layer_resolution = glGetUniformLocation(layer->program, "prev_layer_resolution");

	glBindBuffer(GL_ARRAY_BUFFER, array_buffer_fullscene_quad);
	glVertexAttribPointer(layer->attr.vertex_coord,
	                      4,
	                      GL_FLOAT,
	                      GL_FALSE, /* normalize */
	                      16,
	                      NULL);
	glEnableVertexAttribArray(layer->attr.vertex_coord);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glUseProgram(0);

	CHECK_GL();
	return 0;
}


/* Graphics */
static int Graphics_SetupInitialState(Graphics *g);


Graphics *Graphics_Create(Graphics_LAYOUT layout,
                          int scaling_numer, int scaling_denom)
{ 
 	Graphics *g;
	Scaling sc;

	g = malloc(sizeof(*g));
	if (!g) {
		handleError("Graphics not created", -1);
	}
    
	sc.numer = scaling_numer;
	sc.denom = scaling_denom;

	g->viewport.x = 0;
	g->viewport.y = 0;
    g->viewport.z = 800;
    g->viewport.w = 600;
	g->layout = layout;
	g->array_buffer_fullscene_quad = 0;
	g->vertex_shader = 0;
	g->texture_wrap_mode = Graphics_WRAP_MODE_REPEAT;
	g->texture_interpolation_mode = Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR;
	g->texture_pixel_format = Graphics_PIXELFORMAT_RGBA8888;
	g->num_render_layer = 0;
	g->window_scaling = sc;
	g->enable_backbuffer = 0;
    g->enable_video = 0;
	g->net_params = 0;
	g->backbuffer_texture_object = 0;
	g->backbuffer_texture_unit = 0;
    g->video_texture_object = 0;
	g->video_texture_unit = 0;
    
	return g;
}

void Graphics_Delete(Graphics *g)
{
	Graphics_DeallocateOffscreen(g);

	CHECK_GL();
	if (g->vertex_shader) {
		glDeleteShader(g->vertex_shader);
	}
	CHECK_GL();
	if (g->array_buffer_fullscene_quad) {
		glDeleteBuffers(1, &g->array_buffer_fullscene_quad);
	}
	CHECK_GL();

	glfwTerminate();
	free(g);
}

void Graphics_InitDisplayData(Graphics *g, Sourceparams_t * sourceparams) {
    g->displaydata.window_width = sourceparams->image_width;
    g->displaydata.window_height = sourceparams->image_height;
    g->displaydata.texture_width = sourceparams->image_width;
    g->displaydata.texture_height = sourceparams->image_height;
    g->displaydata.bytes_per_pixel = 2;                 // int 2 for YUV422
    // this can be switched in realtime (TODO)
    //g->displaydata.internal_format = (GLint)GL_RGBA;
    //g->displaydata.pixelformat = (GLenum)GL_RGBA;
    g->displaydata.internal_format = (GLint)GL_RG;
    g->displaydata.pixelformat = (GLenum)GL_RG;
}

static int Graphics_SetupInitialState(Graphics *g)
{
	CHECK_GL();

	if (g->array_buffer_fullscene_quad == 0) {
		static const GLfloat fullscene_quad[] = {
			-1.0, -1.0, 1.0, 1.0,
			1.0, -1.0, 1.0, 1.0,
			1.0,  1.0, 1.0, 1.0,
			-1.0,  1.0, 1.0, 1.0
		};
		glGenBuffers(1, &g->array_buffer_fullscene_quad);
		glBindBuffer(GL_ARRAY_BUFFER, g->array_buffer_fullscene_quad);
		glBufferData(GL_ARRAY_BUFFER, sizeof(fullscene_quad),
		             fullscene_quad, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		CHECK_GL();
	}


	if (g->vertex_shader == 0) {
		GLint param;
		static const GLchar *vertex_shader_source =
		    "attribute vec4 vertex_coord;"
		    "void main(void) { gl_Position = vertex_coord; }";
		g->vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(g->vertex_shader, 1, &vertex_shader_source, NULL);
		glCompileShader(g->vertex_shader);
		glGetShaderiv(g->vertex_shader, GL_COMPILE_STATUS, &param);
		if (param != GL_TRUE) {
			PrintShaderLog("vertex_shader", g->vertex_shader, g->render_layer->attr.lines_before_include, g->render_layer->attr.lines_included);
			assert(0);
			return 1;
		}
		CHECK_GL();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glDisable(GL_SAMPLE_COVERAGE);

	return 0;
}

void Graphics_SetupViewport(Graphics *g) {
    
    int i, count, xpos, ypos;
	
    GLFWmonitor** monitors = glfwGetMonitors(&count);
	for (i = 0; i < count; i++) {
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
		fprintf(stderr, "Monitor[%i]: %i x %i @ %i hz\n", i, mode->width, mode->height, mode->refreshRate);
	}
        
    xpos = 0;
    ypos = 0;
 
    switch (g->layout) {
        case Graphics_LAYOUT_PRIMARY_RESOLUTION: /* default */
            printf("Using primary monitor\n");
        break;
        case Graphics_LAYOUT_PRIMARY_FULLSCREEN:
            printf("Using primary monitor in fullscreen mode\n");
            const GLFWvidmode* mode_primary = glfwGetVideoMode(monitors[0]);
            g->viewport.z = mode_primary->width;
            g->viewport.w = mode_primary->height;
            glfwWindowHint(GLFW_DECORATED,GL_FALSE);
        break;
        case Graphics_LAYOUT_SECONDARY_FULLSCREEN:
            if (count > 1) {
                printf("Using secondary monitor in fullscreen mode\n");
                glfwGetMonitorPos(monitors[1], &xpos, &ypos);
                printf("Placing render window at monitor[1] position: %d x %d \n", xpos, ypos);
                const GLFWvidmode* mode_secondary = glfwGetVideoMode(monitors[1]);
                g->viewport.z = mode_secondary->width;
                g->viewport.w = mode_secondary->height;
                glfwWindowHint(GLFW_DECORATED,GL_FALSE);
            } else {
                printf("Cant detect any secondary monitors\n");
                exit(0);
            }
        break;
        case Graphics_LAYOUT_SECONDARY_RESOLUTION:
            if (count > 1) {
                printf("Using secondary monitor\n");
                glfwGetMonitorPos(monitors[1], &xpos, &ypos);
                printf("Placing render window at monitor[1] position: %d x %d \n", xpos, ypos);
            } else {
                printf("Cant detect any secondary monitors\n");
                exit(0);
            }
        break;
    }
    
	g->window = glfwCreateWindow(g->viewport.z, g->viewport.w, "glslr", NULL, NULL);
    
	if(!g->window) {
		glfwTerminate();
		handleError("GLFW create window failed", -1);
	}
    
    g->viewport.z *= fixDpiScale(g->window);
    g->viewport.w *= fixDpiScale(g->window);
    glfwSetWindowSize(g->window, g->viewport.z, g->viewport.w);
    glfwSetWindowPos(g->window, xpos, ypos);
    Graphics_setWindowSize(g->viewport.z, g->viewport.z);
    glfwMakeContextCurrent(g->window);
    glfwSetWindowSizeCallback(g->window, handleResize);
    glfwSwapInterval(1);
    
	Graphics_SetupInitialState(g);
}

static int Graphics_ApplyWindowChange(Graphics *g)
{
	int width, height;
	int scaled_width, scaled_height;

	{
		//int x, y;
		//int screen_width, screen_height;
		//screen_width = screen_height = 0;
		width = height = 0;
		//x = y = 0;
		// review this for a working scaling - compare to master
		Graphics_GetWindowSize(g, &width, &height);
		scaled_width = width;
		scaled_height = height;
		Scaling_Apply(&g->window_scaling, &scaled_width, &scaled_height);  //gets the image source size
		// ??_setSourceSize(scaled_width, scaled_height); // missing
		Graphics_setWindowSize(width, height);
	}
	CHECK_GL();

	Graphics_SetupInitialState(g);
	Graphics_DeallocateOffscreen(g);
	Graphics_AllocateOffscreen(g);
	return 0;
//damn:
	//return 1;
}

void Graphics_setWindowSize(int _width, int _height)
{
	glViewport(0.0,0.0,(float)_width,(float)_height);
}

void Graphics_SetupWindowSize(Graphics *g, int _width, int _height) // just for testing
{
    g->viewport.z = _width;
    g->viewport.w = _height;
    glfwSetWindowSize(g->window, _width, _height);
}

void Graphics_setSourceSize(int _width, int _height)
{
	glViewport(0.0,0.0,(float)_width,(float)_height);
}

int Graphics_AppendRenderLayer(Graphics *g,
                               const char *source,
                               int lines_before,
                               int lines_included,
                               OPTIONAL int source_length,
                               OPTIONAL void *auxptr)
{
	RenderLayer *layer;

	if (g->num_render_layer >= MAX_RENDER_LAYER) {
		return 1;
	}

	layer = &g->render_layer[g->num_render_layer];
	if (RenderLayer_Construct(layer, lines_before, lines_included, auxptr)) {
		return 2;
	}

	if (RenderLayer_UpdateShaderSource(layer, source, source_length)) {
		RenderLayer_Destruct(layer);
		return 3;
	}
	g->num_render_layer += 1;
	return 0;
}

RenderLayer *Graphics_GetRenderLayer(Graphics *g, int layer_index)
{
	assert(layer_index >= 0);
	if (layer_index >= g->num_render_layer) {
		return NULL;
	}
	return &g->render_layer[layer_index];
}

void Graphics_SetOffscreenPixelFormat(Graphics *g, Graphics_PIXELFORMAT pixel_format)
{
	g->texture_pixel_format = pixel_format;
}

void Graphics_SetOffscreenInterpolationMode(Graphics *g, Graphics_INTERPOLATION_MODE interpolation_mode)
{
	g->texture_interpolation_mode = interpolation_mode;
}

void Graphics_SetOffscreenWrapMode(Graphics *g, Graphics_WRAP_MODE wrap_mode)
{
	g->texture_wrap_mode = wrap_mode;
}

int Graphics_ApplyOffscreenChange(Graphics *g)
{
	Graphics_DeallocateOffscreen(g);
	return Graphics_AllocateOffscreen(g);
}

void Graphics_SetLayout(Graphics *g, Graphics_LAYOUT layout, int width, int height)
{
    g->layout = layout;
    g->viewport.z = width;
    g->viewport.w = height;
}

void Graphics_SetWindowScaling(Graphics *g, int numer, int denom)
{
	assert(numer >= 0);
	assert(denom >= 0);
	assert(numer <= denom);

	g->window_scaling.numer = numer;
	g->window_scaling.denom = denom;
}

int Graphics_ApplyWindowScalingChange(Graphics *g)
{
	return Graphics_ApplyWindowChange(g);
}

int Graphics_AllocateOffscreen(Graphics *g)
{
	int i;
	int source_width, source_height;

	Graphics_getWindowWidth(g, &source_width);
	Graphics_getWindowHeight(g, &source_height);

	CHECK_GL();
	for (i = 0; i < g->num_render_layer; i++) {
		RenderLayer *layer = &g->render_layer[i];
		int texture_unit = i;
		int is_final_layer = (i == (g->num_render_layer - 1)) ? 1 : 0;
		RenderLayer_AllocateOffscreen(layer, is_final_layer, texture_unit,
		                              source_width, source_height,
		                              g->texture_pixel_format,
		                              g->texture_interpolation_mode,
		                              g->texture_wrap_mode);
		/* TODO: handle error */
	}
    glGenTextures(2, &g->textures);
    g->video_texture_object = g->textures[0];
    g->backbuffer_texture_object = g->textures[1];
    if (g->enable_video) {
        GLint internal_format = (GLint)g->displaydata.internal_format;
        GLenum pixelformat = (GLenum)g->displaydata.pixelformat;
        g->video_texture_unit = g->num_render_layer * 2;
        glBindTexture(GL_TEXTURE_2D, g->video_texture_object);
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     internal_format,
                     g->displaydata.texture_width, g->displaydata.texture_height,
                     0,
                     pixelformat, 
                     GL_UNSIGNED_BYTE, 
                     NULL);  
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
        glBindTexture(GL_TEXTURE_2D, 0);                        // fix this too
    }
	if (g->enable_backbuffer) {
		GLint internal_format;
		GLenum format;
		GLenum type;
		DeterminePixelFormat(g->texture_pixel_format, &internal_format, &format, &type);
        g->backbuffer_texture_unit = (g->num_render_layer * 2) + 1;
		glBindTexture(GL_TEXTURE_2D, g->backbuffer_texture_object);
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             internal_format,
		             source_width, source_height,
		             0,
		             format,
		             type,
		             NULL);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	CHECK_GL();
	return 0;
}

void Graphics_DeallocateOffscreen(Graphics *g)
{
	int i;
	if (g->backbuffer_texture_object) {
		glDeleteTextures(1, &g->backbuffer_texture_object);
		g->backbuffer_texture_object = 0;
	}
    if (g->video_texture_object) {
		glDeleteTextures(1, &g->video_texture_object);
		g->video_texture_object = 0;
	}
	for (i = g->num_render_layer - 1; i >= 0; i--) {
		RenderLayer_DeallocateOffscreen(&g->render_layer[i]);
	}
}

int Graphics_BuildRenderLayer(Graphics *g, int layer_index)
{
    RenderLayer_BuildProgram(&g->render_layer[layer_index],
	                         g->vertex_shader,
	                         g->array_buffer_fullscene_quad,
	                         g->net_params);
	/* TODO: handle error */
	return 0;
}

void Graphics_SetUniforms(Graphics *g, double t,
                          netin_val *net_input_val,
                          double mouse_x, double mouse_y,
                          double randx, double randy)
{
	int i,j;
	int width, height;
	netin_val *val=NULL;
	netin_addr *addr=NULL;

	CHECK_GL();
	Graphics_GetSourceSize(g, &width, &height);
	for (i = 0; i < g->num_render_layer; i++) {
		RenderLayer *p;
		p = &g->render_layer[i];
		glUseProgram(p->program);
		glUniform1f(p->attr.time, t);
		glUniform2f(p->attr.resolution, (double)width, (double)height);

		// fills m_{0,net_params}) adresses with values
		j = 0;
		val = net_input_val;
		addr = p->attr.net_input_addr;
		while (j < g->net_params) {
			glUniform1f(addr->addr, val->val);
			val = val->next;
			addr = addr->next;
			j++;
		}

		glUniform2f(p->attr.mouse, mouse_x, mouse_y);
		glUniform2f(p->attr.rand, randx, randy);

		glUseProgram(0);
	}
	CHECK_GL();
}

void Graphics_Render(Graphics *g, Sourceparams_t * sourceparams) {
	int i;
	GLuint prev_layer_texture_unit;
	GLuint prev_layer_texture_object;

	CHECK_GL();
	prev_layer_texture_unit = 0;
	prev_layer_texture_object = 0;
	for (i = 0; i < g->num_render_layer; i++) {
		RenderLayer *p;
		p = &g->render_layer[i];
		glUseProgram(p->program);
		if (g->enable_backbuffer) {
			glUniform1i(p->attr.backbuffer, g->backbuffer_texture_unit);
			glActiveTexture(GL_TEXTURE0 + g->backbuffer_texture_unit);
			glBindTexture(GL_TEXTURE_2D, g->backbuffer_texture_object);
		}
        if (g->enable_video) {
			glUniform1i(p->attr.video, g->video_texture_unit);
			glActiveTexture(GL_TEXTURE0 + g->video_texture_unit);
			glBindTexture(GL_TEXTURE_2D, g->video_texture_object);
		}
        if (i == 0) {
			/* primary layer */
			glBindFramebuffer(GL_FRAMEBUFFER, p->framebuffer);
			glActiveTexture(GL_TEXTURE0 + p->texture_unit);
			glBindTexture(GL_TEXTURE_2D, 0);
		} else if (i == (g->num_render_layer-1)) {
			/* final layer */
			glUniform1i(p->attr.prev_layer, prev_layer_texture_unit);
			/* TODO: plav_layer_resolution */
			glActiveTexture(GL_TEXTURE0 + prev_layer_texture_unit);
			glBindTexture(GL_TEXTURE_2D, prev_layer_texture_object);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		} else {
			glUniform1i(p->attr.prev_layer, prev_layer_texture_unit);
			/* TODO: plav_layer_resolution */
			glActiveTexture(GL_TEXTURE0 + p->texture_unit);
			glBindTexture(GL_TEXTURE_2D, 0);
			glActiveTexture(GL_TEXTURE0 + prev_layer_texture_unit);
			glBindTexture(GL_TEXTURE_2D, prev_layer_texture_object);
			glBindFramebuffer(GL_FRAMEBUFFER, p->framebuffer);
		}

		glBindBuffer(GL_ARRAY_BUFFER, g->array_buffer_fullscene_quad);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glFlush();

		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		glUseProgram(0);

		prev_layer_texture_unit = p->texture_unit;
		prev_layer_texture_object = p->texture_object;
	}

	if (g->enable_backbuffer) {
		int width, height;
		Graphics_GetSourceSize(g, &width, &height);
		glActiveTexture(GL_TEXTURE0 + g->backbuffer_texture_unit);
		glBindTexture(GL_TEXTURE_2D, g->backbuffer_texture_object);
		glBindFramebuffer(GL_FRAMEBUFFER, g->render_layer[g->num_render_layer-1].framebuffer);

		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, width, height, 0);

		glActiveTexture(GL_TEXTURE0 + g->backbuffer_texture_unit);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
    if (g->enable_video) {
        //int texture_size;
        //g->displaydata.bytes_per_pixel = 2;
        //texture_size = g->displaydata.texture_width * g->displaydata.texture_height * g->displaydata.bytes_per_pixel;
        //g->displaydata.texture = malloc(texture_size);
        GLint internal_format = (GLint)g->displaydata.internal_format;
        GLenum pixelformat = (GLenum)g->displaydata.pixelformat;
		glActiveTexture(GL_TEXTURE0 + g->video_texture_unit);
		glBindTexture(GL_TEXTURE_2D, g->video_texture_object); 
		glTexImage2D(GL_TEXTURE_2D,
                     0,             /* level */
                     internal_format,
                     g->displaydata.texture_width, g->displaydata.texture_height,
                     0,             /* border */
                     pixelformat, 
                     GL_UNSIGNED_BYTE, 
                     sourceparams->captured.start);
        //Swizzle mask: https://www.opengl.org/wiki/Texture#Swizzle_mask
        // u, y1, v, y2
        //GLint swizzleMask[] = {GL_GREEN, GL_RED, GL_BLUE, GL_ONE};           // RGB1 - y1,u,v
        //GLint swizzleMask[] = {GL_ALPHA, GL_RED, GL_BLUE, GL_ONE};           // RGB2 - y2,u,v
        //glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		glActiveTexture(GL_TEXTURE0 + g->video_texture_unit);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	CHECK_GL();

	glfwSwapBuffers(g->window);
}

void Graphics_SetBackbuffer(Graphics *g, int enable)
{
	g->enable_backbuffer = enable;
}

void Graphics_SetVideo(Graphics *g, int enable)
{
	g->enable_video = enable;
}

void Graphics_SetNetParams(Graphics *g, int params)
{
	g->net_params = params;
}

void Graphics_GetWindowSize(Graphics *g, int *_width, int *_height)
{
	glfwGetWindowSize(g->window, _width, _height);
}

void Graphics_GetSourceSize(Graphics *g, int *width, int *height)
{
	// Video_GetSourceSize(g->video, out_width, out_height);
	Graphics_getWindowHeight(g, height);
	Graphics_getWindowWidth(g, width);
}


static void DeterminePixelFormat(Graphics_PIXELFORMAT pixel_format,
                                 GLint *out_internal_format,
                                 GLenum *out_format, GLenum *out_type)
{
	GLint internal_format;
	GLenum format;
	GLenum type;

	CHECK_GL();

	switch (pixel_format) {
		/* 32bpp */
	case Graphics_PIXELFORMAT_RGBA8888:
		internal_format = GL_RGBA;
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
		/* 24bpp */
	case Graphics_PIXELFORMAT_RGB888:
		internal_format = GL_RGB;
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		break;
		/* 16bpp */
	case Graphics_PIXELFORMAT_RGB565:
		internal_format = GL_RGB;
		format = GL_RGB;
		type = GL_UNSIGNED_SHORT_5_6_5;
		break;
	case Graphics_PIXELFORMAT_RGBA5551:
		internal_format = GL_RGBA;
		format = GL_RGBA;
		type = GL_UNSIGNED_SHORT_5_5_5_1;
		break;
	case Graphics_PIXELFORMAT_RGBA4444:
		internal_format = GL_RGBA;
		format = GL_RGBA;
		type = GL_UNSIGNED_SHORT_4_4_4_4;
		break;
	default:
		assert(!"invalid pixel format");
		internal_format = GL_RGBA;
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		break;
	}
	*out_internal_format = internal_format;
	*out_format = format;
	*out_type = type;
}

void Graphics_getWindowWidth(Graphics *g, int *width)
{
	*width = g->viewport.z;
}

void Graphics_getWindowHeight(Graphics *g, int *height)
{
	*height = g->viewport.w;
}
