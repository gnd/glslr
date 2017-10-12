/// libcurl creates a connection and calls the callback function (in this case JPEGMemoryCallbackClean) on data reception
/// the function first read first 136 bytes to get the jpeg data size and then reads the rest of the data
/// after it finishes reading the desired amount of dat, it saves the file to disk & creates a libjpeg object 
/// then it makes mem->size = 0 and repeats the process
///
/// created at koumaria residency, 2017
/// fuck im tired, everyone just works here, no fun :(
///
/// TODO
/// add a simple opengl shader with a couple options (multiscreen + resolution) to easily watch the data
/// add an option to view & save the data to disk (see *Saver callback)
//
// compile like gcc -o lc libcurl_sony_test.c -lcurl -ljpeg

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <jpeglib.h>
#include <jerror.h>
#include <curl/curl.h>
#include <netinet/in.h>

size_t global_size = 0;
char *payload_header;
bool header_found = false;
char size_string[6];
size_t jpeg_size;
bool jpeg_written = false;
size_t padding_size;
struct timeval tv1, tv2;
int picture_number = 0;

/// libjpeg
unsigned long x;
unsigned long y;
unsigned short int bpp; //bits per pixels   unsigned short int
char* data;             //the data of the image
unsigned long size;     //length of the file
int channels;      //the channels of the image 3 = RGA 4 = RGBA


struct MemoryStruct {
  char *memory;
  size_t size;
};


bool LoadJPEG(char * imgdata, bool fast) {

  struct jpeg_decompress_struct info;  //the jpeg decompress info
  struct jpeg_error_mgr err;           //the error handler
 
  info.err = jpeg_std_error(&err);     //tell the jpeg decompression handler to send the errors to err
  jpeg_create_decompress(&info);       //sets info to all the default stuff
  jpeg_mem_src(&info, imgdata, jpeg_size);
  jpeg_read_header(&info, TRUE);   //tell it to start reading it

  if(fast) {
    info.do_fancy_upsampling = FALSE;
  }
 
  jpeg_start_decompress(&info);    //decompress the file
 
  x = info.output_width;
  y = info.output_height;
  channels = info.num_components;
 
  /* this makes sense only with opengl
  //type = GL_RGB;
 
  if(channels == 4) {
    type = GL_RGBA;
  }
  */
 
  bpp = channels * 8;
  size = x * y * 3;
  data = malloc(size); 
  char* p1 = data;
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
   * */
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
  /**/
  
  return true;
}


/* The prototype of the callback function 
 * 
 * 
 */
static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  printf("Entering callback\n");
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }
 
  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  global_size += realsize;
  printf("Read so much: %lu\n", global_size);

  printf("Returning\n");
 
  return realsize;
}



/* This connects to the Action CAM and:
 * - saves the image stream to disk as separate images
 * - outputs the time taken to save these
 * 
 *  TODO:
 *  - provide path where to save and filename
 */
static size_t JPEGMemoryCallbackSaver(void *contents, size_t size, size_t nmemb, void *userp) {
        size_t realsize = size * nmemb;
        struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // measure time spent on all of this
	if (mem->size < 1) {
		gettimeofday(&tv2, NULL);
	}

    // get more memory space 
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    // copy contents to memory
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    // did we read the payload header ?
    if (mem->size == 136) {
        payload_header = malloc(136);
        memcpy(&payload_header, &mem->memory, 136);

        sprintf(size_string,"%02hhx%02hhx%02hhx", (unsigned char) payload_header[12], (unsigned char) payload_header[13], (unsigned char) payload_header[14]);
        jpeg_size = (int)strtol(size_string, NULL, 16);

        sprintf(size_string,"%02hhx", (unsigned char) payload_header[15]);
        padding_size = (int)strtol(size_string, NULL, 16);

        header_found = true;
	}
        
	if ((mem->size >= 136 + jpeg_size + padding_size) && header_found) {
		if (!jpeg_written) {
			//printf("Size: %lu, nmemb: %lu, mem->size: %lu, realsize: %lu, jpeg_size: %lu, sizeof(memory): %lu\n", size, nmemb, mem->size, realsize, jpeg_size, sizeof(&mem->memory));

			char filename[15];
			sprintf(filename, "sony_%05d.jpeg", picture_number);
			FILE *f = fopen(filename, "w");
 			if (f == NULL) {
				printf("Error opening file!\n");
				exit(1);
			}
			fwrite(&mem->memory[136], jpeg_size, 1, f);
			fclose(f);
            
			gettimeofday(&tv2, NULL);
			picture_number++;
			printf("Picture %05d downloaded in %f seconds\n", picture_number, (double) (tv2.tv_usec - tv1.tv_usec) / 1000000 + (double) (tv2.tv_sec - tv1.tv_sec));
			gettimeofday(&tv1, NULL);
			
            // zero the size when u want to continuously rewrite the chunk with new data (and save it)
            mem->size=0;

			return realsize;
		}
	}
    return realsize;
}



/* This connects to the Action CAM and:
  * - just downloads one image and exits with an error (not the right realsize returned)
  *
  * 
  * 
  * */
static size_t JPEGMemoryCallbackOneShot(void *contents, size_t size, size_t nmemb, void *userp) {
        size_t realsize = size * nmemb;
        struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    // get more memory space 
    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }
    // copy contents to memory
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    // did we read the payload header ?
    if (mem->size == 136) {
        payload_header = malloc(136);
        memcpy(&payload_header, &mem->memory, 136);

        sprintf(size_string,"%02hhx%02hhx%02hhx", (unsigned char) payload_header[12], (unsigned char) payload_header[13], (unsigned char) payload_header[14]);
        jpeg_size = (int)strtol(size_string, NULL, 16);

        header_found = true;
        printf("HeADER FOUND\n");
	}
        
	if ((mem->size >= 136 + jpeg_size) && header_found) {
        printf("trying to exit\n");
        // exit with an error (to stop libcurl from downloading the stream)
        return realsize-1;
	}
    return realsize;
}


 
int main(void) {
    CURL *curl_handle;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;
     
    curl_global_init(CURL_GLOBAL_ALL);
    curl_handle = curl_easy_init();
    curl_easy_setopt(curl_handle, CURLOPT_URL, "http://192.168.122.1:60152/liveview.JPG?!1234!http-get:*:image/jpeg:*!!!!!");
    // OneShot is just for testing libjpeg functionality
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, JPEGMemoryCallbackOneShot);
    //curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, JPEGMemoryCallbackSaver);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
    res = curl_easy_perform(curl_handle);
     
    /* actually this is for loading one picture, we need to finish callback with an error
     * next step should be continuously rewriting the chunk.memory with image data in a standalone thread
     * and using some kind of signalling that a new picture is available
     */
    if(res != CURLE_OK) {
        //fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        //printf("Loading JPEG data into libjpeg\n");
        LoadJPEG(&chunk.memory[136], true);
    } else {
        printf("%lu bytes retrieved\n", (long)chunk.size);
    }
    curl_easy_cleanup(curl_handle);
    free(chunk.memory);
    curl_global_cleanup();
    return 0;
}
