#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include <fcntl.h>
#include <termios.h>

#include "config.h"
#include "base.h"
#include "graphics.h"
#include <stdbool.h>
#include <pthread.h>

#include "v4l2_controls.h"
#include "util_textfile.h"

/* pdreceive includes */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <curl/curl.h>

typedef struct Glslr_ Glslr;
void udpmakeoutput(char *buf, Glslr *gx);

int Glslr_HostInitialize(void);
void Glslr_HostDeinitialize(void);

size_t Glslr_InstanceSize(void);
int Glslr_Construct(Glslr *gx);
void Glslr_Destruct(Glslr *gx);
int Glslr_ParseArgs(Glslr *gx, int argc, const char *argv[]);
int Glslr_Main(Glslr *gx);
void Glslr_Usage(void);
int Glslr_GetLineCount(char *code, size_t size);
void Glslr_IncludeAdditionalCode(char *code, int *len, int *lines_before, int *lines_included);

// TODO define in only one headerfile
void *getJpegData(void *memory);
typedef struct JpegMemory_s {
    CURL *curl_handle;
    unsigned char *memory;
    char *size_string;
    size_t size;
    size_t jpeg_size;
    bool header_found;
    CURL *handle;
} JpegMemory_t;
