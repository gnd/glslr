#include <stdbool.h>

typedef struct JpegMemory_s {
    char *memory;
    size_t size;
    bool header_found;
    char *size_string;
    size_t jpeg_size;
} JpegMemory_t;

typedef struct JpegDec_s {
        unsigned long x;
        unsigned long y;
        unsigned short int bpp; //bits per pixels   unsigned short int
        char* data;             //the data of the image
        unsigned long size;     //length of the file
        int channels;      //the channels of the image 3 = RGA 4 = RGBA
} JpegDec_t;

static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void *userp);
//static size_t SonyCallback(void *contents, size_t size, size_t nmemb, JpegMemory_t* mem);
void getJpegData(JpegMemory_t* mem);
void LoadJPEG(char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size);