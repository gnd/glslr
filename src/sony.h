#include <stdbool.h>
#include <curl/curl.h>

typedef struct JpegMemory_s {
    char *memory;
    const unsigned char *ro_memory;
    char *size_string;
    size_t size;
    size_t jpeg_size;
    size_t *ro_jpeg_size;
    bool header_found;
    bool image_read;
    bool image_getting;
    bool image_swapping;
    int identifier;
    CURL *handle;
} JpegMemory_t;

typedef struct JpegDec_s {
        unsigned long x;
        unsigned long y;
        unsigned short int bpp; //bits per pixels   unsigned short int
        unsigned char* data;             //the data of the image
        unsigned long size;     //length of the file
        int channels;      //the channels of the image 3 = RGA 4 = RGBA
} JpegDec_t;

//static size_t SonyCallback(void *contents, size_t size, size_t nmemb, JpegMemory_t* mem);
void *getJpegData(void *memory);
void *getJpegDataThread(void *handle);
void *getJpegDataThreadNext(void *memory);
void LoadJPEG(const unsigned char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size);
