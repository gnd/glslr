#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <jpeglib.h>
#include <jerror.h>
#include "sony.h"

// TODO move this to the headerfile 
static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void* userp);

static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    JpegMemory_t *mem = (JpegMemory_t *)userp;

    if (!mem->image_read) {
      mem->memory = realloc(mem->memory, mem->size + realsize + 1);
      if(mem->memory == NULL) {
          printf("Not enough memory (realloc returned NULL)\n");
          return 0;
      }

      memcpy(&(mem->memory[mem->size]), contents, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
    }

    // did we read the payload header ?
    if ((mem->size == 136) && !(mem->image_read)) {
        mem->size_string = malloc(16);
        sprintf(mem->size_string,"%02hhx%02hhx%02hhx", (unsigned char) mem->memory[12], (unsigned char) mem->memory[13], (unsigned char) mem->memory[14]);
        mem->jpeg_size = (int)strtol(mem->size_string, NULL, 16);
        mem->header_found = true;
    }

    // read the jpeg data
    if ((mem->size == 136 + mem->jpeg_size) && mem->header_found) {
        mem->image_swapping = true;
        *mem->ro_jpeg_size = mem->jpeg_size;
        mem->ro_memory = malloc(*mem->ro_jpeg_size);
        memcpy(&mem->ro_memory, mem->memory, *mem->ro_jpeg_size);
        mem->image_swapping = false;
        mem->image_read = true;
        mem->header_found = false;
        mem->size = 0;
    }
    return realsize;
}


void *getJpegDataThread(void *handle) {

    CURL *curl_handle = (CURL *)handle;
    curl_easy_perform(curl_handle);
    //curl_easy_cleanup(curl_handle);
    printf("am i ever here ?\n");
    //curl_global_cleanup();

    // make gcc happy
    return 0;
}



void *getJpegDataThreadNext(void *memory) {
    printf("Thread[]: Setting up callback\n");
    JpegMemory_t *chunk = (JpegMemory_t *)memory;

    curl_easy_setopt(chunk->handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(chunk->handle, CURLOPT_WRITEFUNCTION, SonyCallback);
    curl_easy_setopt(chunk->handle, CURLOPT_WRITEDATA, chunk);
    curl_easy_perform(chunk->handle);
    printf("lalal\n");
    curl_easy_cleanup(chunk->handle);
    curl_global_cleanup();

    // make gcc happy
    return 0;
}


/* this loads the JPEG data into the char* data array */
void LoadJPEG(const unsigned char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size) {

  printf("Entered LoadJPEG, jpeg size is %lu\n", jpeg_size);

  struct jpeg_decompress_struct info;  //the jpeg decompress info
  struct jpeg_error_mgr err;           //the error handler

  info.err = jpeg_std_error(&err);     //tell the jpeg decompression handler to send the errors to err
  jpeg_create_decompress(&info);       //sets info to all the default stuff
  jpeg_mem_src(&info, imgdata, jpeg_size);
  jpeg_read_header(&info, TRUE);   //tell it to start reading it

  printf("setting up some variables\n");
  // speed things up ?
  info.do_fancy_upsampling = FALSE;

  printf("going to decompress now\n");
  jpeg_start_decompress(&info);    //decompress the file

  printf("decompressed ! \n");
  jpeg_dec->x = info.output_width;
  jpeg_dec->y = info.output_height;
  jpeg_dec->channels = info.num_components;

  /* this makes sense only with opengl
  //type = GL_RGB;

  if(channels == 4) {
    type = GL_RGBA;
  }
  */

  jpeg_dec->bpp = jpeg_dec->channels * 8;
  jpeg_dec->size = jpeg_dec->x * jpeg_dec->y * 3;
  jpeg_dec->data = malloc(jpeg_dec->size);
  unsigned char* p1 = jpeg_dec->data;
  unsigned char** p2 = &p1;
  int numlines = 0;

  while(info.output_scanline < info.output_height) {
    numlines = jpeg_read_scanlines(&info, p2, 1);
    *p2 += numlines * 3 * info.output_width;
  }
  jpeg_finish_decompress(&info);
}
