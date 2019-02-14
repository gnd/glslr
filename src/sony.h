#include <stdbool.h>
#include <curl/curl.h>

typedef struct JpegMemory_s {
    CURL *curl_handle;
    unsigned char *memory;
    char *size_string;
    size_t size;
    size_t jpeg_size;
    bool header_found;
    CURL *handle;
} JpegMemory_t;

typedef struct JpegDec_s {
        unsigned long x;
        unsigned long y;
        unsigned short int bpp;
        unsigned char* data;
        unsigned long size;
        int channels;
} JpegDec_t;

extern pthread_mutex_t video_mutex;
extern JpegDec_t jpeg_dec;
static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void *userp);
void LoadJPEG(const unsigned char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size);
