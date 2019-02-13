#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <jpeglib.h>
#include <jerror.h>
#include "sony.h"


static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void* userp);


static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void* userp) {
    printf("Thread[]: Entering callback (1)\n");
    size_t realsize = size * nmemb;
    printf("Thread[]: Entering callback (2)\n");
    JpegMemory_t *mem = (JpegMemory_t *)userp;
    printf("Thread[%d]: Entering callback (3)\n", mem->identifier);

    if (mem->identifier == 0) {
        mem->identifier = rand();
        printf("Thread[%d]: Now im thread %d, realsize is %ld, mem->size is %ld, image_read is: %d\n", mem->identifier, mem->identifier, realsize, mem->size, mem->image_read);
    } else {
        printf("Thread[%d]: I am still thread %d, realsize is %ld, mem->size is %ld, image_read is: %d\n", mem->identifier, mem->identifier, realsize, mem->size, mem->image_read);
    }

    if (!mem->image_read) {
      //printf("in SonyCallback, my identifier is %d memsize is %lu, image-read is %d\n", mem->identifier, mem->size, mem->image_read);
      mem->memory = realloc(mem->memory, mem->size + realsize + 1);
      if(mem->memory == NULL) {
          printf("Not enough memory (realloc returned NULL)\n");
          return 0;
      }

      //printf("resizing memory by %lu\n", realsize);
      printf("Thread[%d]: Copying memory\n", mem->identifier);
      memcpy(&(mem->memory[mem->size]), contents, realsize);
      printf("Thread[%d]: Resizing mem->size to %ld\n", mem->identifier, mem->size + realsize);
      mem->size += realsize;
      printf("Thread[%d]: Null terminating memory\n", mem->identifier);
      mem->memory[mem->size] = 0;
      printf("Thread[%d]: Memory resize done\n", mem->identifier);
    }

    // did we read the payload header ?
    if ((mem->size == 136) && !(mem->image_read)) {
        //printf("header found\n");
        //printf("%02hhx%02hhx%02hhx\n", (unsigned char) mem->memory[12], (unsigned char) mem->memory[13], (unsigned char) mem->memory[14]);
        mem->size_string = malloc(16);
        sprintf(mem->size_string,"%02hhx%02hhx%02hhx", (unsigned char) mem->memory[12], (unsigned char) mem->memory[13], (unsigned char) mem->memory[14]);
        //printf("String created\n");
        mem->jpeg_size = (int)strtol(mem->size_string, NULL, 16);
        //printf("Setting jpeg size to %lu\n", mem->jpeg_size);
        mem->header_found = true;
    }

    // read the jpeg data
    if ((mem->size == 136 + mem->jpeg_size) && mem->header_found) {
        //printf("image read, going to overwrite read-only memory\n");
        mem->image_swapping = true;
        //printf("ro_jpeg_size is now: %lu\n", *mem->ro_jpeg_size);
        printf("Thread[%d]: making ro_jpeg_size like this: %lu\n", mem->identifier, mem->jpeg_size);
        *mem->ro_jpeg_size = mem->jpeg_size;
        printf("Thread[%d]: ro_jpeg_size is now: %lu\n", mem->identifier, *mem->ro_jpeg_size);
        //printf("Allocating ro_memory space of size %d\n", *mem->ro_jpeg_size);
        mem->ro_memory = malloc(*mem->ro_jpeg_size);
        //printf("Copying to ro_memory space\n");
        memcpy(&mem->ro_memory, mem->memory, *mem->ro_jpeg_size);
        //printf("ro_memory space ready\n");
        mem->image_swapping = false;
        printf("Thread[%d]: setting image_read true\n", mem->identifier);
        mem->image_read = true;
        mem->header_found = false;
        mem->size = 0;
        printf("Thread[%d]: Another image found\n", mem->identifier);
        //return realsize;
    }
    printf("Thread[%d]: Exiting callback\n", mem->identifier);
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


/*
void *getJpegData(void *memory) {
    JpegMemory_t *chunk = (JpegMemory_t *)memory;
    printf("getJpegData started\n");

    CURL *curl_handle;

    // setup libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://192.168.122.1:60152/liveview.JPG?!1234!http-get:*:image/jpeg:*!!!!!");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, SonyCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, chunk);

    // we make sure this finishes after one frame
    printf("Going to easy_perform\n");
    curl_easy_perform(curl_handle);
    printf("Easy perform done\n");

    // cleanup
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}
*/


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
  printf("JPEG loaded successfully ! \n");

  /* this is just to test if libjpeg actually works
   *
   *
  printf("Trying to save it\n");
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *outfile = fopen( "file.jpeg", "wb" );
  cinfo.err = jpeg_std_error( &jerr );
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo, outfile);
  cinfo.image_width = 640;//width;
  cinfo.image_height = 360;//height;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  jpeg_set_defaults( &cinfo );
  jpeg_start_compress( &cinfo, TRUE );
  JSAMPROW buffer;
  while (cinfo.next_scanline < cinfo.image_height) {
        JSAMPLE *row = data + 3 * cinfo.image_width * cinfo.next_scanline;
        jpeg_write_scanlines(&cinfo, &row, 1);
    }
  jpeg_finish_compress( &cinfo );
  jpeg_destroy_compress( &cinfo );
  fclose( outfile );
  printf("Saved file ! \n");
  */
}

/* for testing
int main(void) {

    // setup memory
    struct JpegMemory chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    // retrieve data from the cam
    getJpegData(&chunk);

    // load it into char* data
    LoadJPEG(&chunk.memory[136], true);

    // do something

    free(chunk.memory);
}
*/
