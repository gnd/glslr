#include <linux/videodev2.h>

#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define MAX_DEVICENAME 80
#define MAX_VIDEO_BUFFERS 4

typedef enum inputsource_e {
  TESTPATTERN, 
  LIVESOURCE
} Inputsource_t;

typedef enum encodingmethod_e {
  LUMA, /* greyscale V4L2_PIX_FMT_GREY  */
  YUV420, /* V4L2_PIX_FMT_YUV420  */
  YUV422, /* V4L2_PIX_FMT_YUYV   */
  /* RGB_BAYER, */
  RGB /* V4L2_PIX_FMT_RGB24  */
} Encodingmethod_t;

typedef struct capabilities_s {
  struct v4l2_capability capture;
  /* struct v4l2_cropcap cropcap; */
  struct v4l2_format fmt;
  int supports_yuv420; /* V4L2_PIX_FMT_YUV420  */
  int supports_yuv422; /* V4L2_PIX_FMT_YUYV  */
  int supports_greyscale; /* V4L2_PIX_FMT_GREY  */
  int supports_rgb; /* V4L2_PIX_FMT_RGB24  */
} Videocapabilities_t;

typedef enum iomethod_e {
  IO_METHOD_READ, /* use read/write to move data around  */
  IO_METHOD_MMAP, /* use mmap to access data  */
  IO_METHOD_USERPTR /* driver stores data in user space  */
} Iomethod_t;

typedef struct videobuffer_s {
  void * start; /* start of the buffer  */
  size_t length; /* buffer length in bytes  */
} Videobuffer_t;


typedef struct sourceparams {
  Inputsource_t source; /* live or test pattern  */
  int fd; /* of open device file or -1 for test pattern  */
  Encodingmethod_t encoding; /* how the data is encoded  */
  int image_width;  /* in pixels  */
  int image_height;  /* in pixels  */
  Iomethod_t iomethod; /* how to get to the data  */
  int buffercount; /* # buffers to juggle  */
  Videobuffer_t buffers[MAX_VIDEO_BUFFERS]; /* where the data is  */ 
  Videobuffer_t captured; /* copied from testpattern or buffers  */
} Sourceparams_t;

void init_device_and_buffers(char * devicename, Sourceparams_t * sourceparams, Videocapabilities_t * capabilities);
int _verify_and_open_device(char * devicename);
void _get_device_capabilities(char * devicename, int device_fd,  Videocapabilities_t * capabilities);
void _collect_supported_image_formats(int device_fd,  Videocapabilities_t * capabilities);
void print_supported_framesizes(int device_fd, __u32 pixel_format, char * label);
void _describe_capture_capabilities(char *errstring, struct v4l2_capability * cap);
int _compute_bytes_per_frame(int image_width, int image_height);
void _try_reset_crop_scale(Sourceparams_t * sourceparams);
int _set_image_size_and_format(Sourceparams_t * sourceparams);
int _request_and_mmap_io_buffers(Sourceparams_t * sourceparams);
int _request_video_buffer_access(int device_fd, enum v4l2_memory memory);
int _mmap_io_buffers(Sourceparams_t * sourceparams);
int _enqueue_mmap_buffers(Sourceparams_t * sourceparams);
int _start_streaming(Sourceparams_t * sourceparams);
int _stop_streaming(Sourceparams_t * sourceparams);