#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <jpeglib.h>
#include <jerror.h>
#include "sony.h"

//static size_t SonyCallback(void *contents, size_t size, size_t nmemb, JpegMemory_t* mem) {
static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    JpegMemory_t *mem = (JpegMemory_t *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    // did we read the payload header ?
    if (mem->size == 136) {
        sprintf(mem->size_string,"%02hhx%02hhx%02hhx", (unsigned char) mem->memory[12], (unsigned char) mem->memory[13], (unsigned char) mem->memory[14]);
        mem->jpeg_size = (int)strtol(mem->size_string, NULL, 16);
        mem->header_found = true;
    }

    // read the jpeg data
    if ((mem->size >= 136 + mem->jpeg_size) && mem->header_found) {
        // exit with an error to close libcurl connection
        return realsize-1;
    }
    return realsize;
}


void getJpegData(JpegMemory_t *chunk) {
    //printf("getJpegData started\n");

    CURL *curl_handle;
    
    // setup libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://192.168.122.1:60152/liveview.JPG?!1234!http-get:*:image/jpeg:*!!!!!");
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, SonyCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, chunk);

    // we make sure this finishes after one frame
    //printf("Going to easy_perform\n");
    curl_easy_perform(curl_handle);

    // cleanup
    curl_easy_cleanup(curl_handle);
    curl_global_cleanup();
}



/* this loads the JPEG data into the char* data array */
void LoadJPEG(char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size) {

  struct jpeg_decompress_struct info;  //the jpeg decompress info
  struct jpeg_error_mgr err;           //the error handler
 
  info.err = jpeg_std_error(&err);     //tell the jpeg decompression handler to send the errors to err
  jpeg_create_decompress(&info);       //sets info to all the default stuff
  jpeg_mem_src(&info, imgdata, jpeg_size);
  jpeg_read_header(&info, TRUE);   //tell it to start reading it

  // speed things up ? 
  info.do_fancy_upsampling = FALSE;
  
 
  jpeg_start_decompress(&info);    //decompress the file
 
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
  char* p1 = jpeg_dec->data;
  char** p2 = &p1;
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