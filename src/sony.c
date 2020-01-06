#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <curl/curl.h>
#include <jpeglib.h>
#include <jerror.h>
#include <pthread.h>
#include "sony.h"
// TODO move headers into sony.h

static size_t SonyCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    JpegMemory_t *mem = (JpegMemory_t *)userp;

    //printf("entenring callback\n");

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
        // Lock mem and read data
        pthread_mutex_lock(&video_mutex);
        LoadJPEG(&mem->memory[136], &jpeg_dec, mem->jpeg_size);
        pthread_mutex_unlock(&video_mutex);

        // start loading another image
        mem->size = 0;

        // check if we should stop
        if (mem->stop) {
            // this will exit the callback
            return realsize-1;
        } else {
            return realsize;
        }
    }
    return realsize;
}

void *getJpegData(void *memory) {
    JpegMemory_t *mem = (JpegMemory_t *)memory;

    CURLcode res;
    char errbuf[CURL_ERROR_SIZE];
    curl_easy_setopt(mem->curl_handle, CURLOPT_ERRORBUFFER, errbuf);
    errbuf[0] = 0;

    curl_easy_setopt(mem->curl_handle, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(mem->curl_handle, CURLOPT_WRITEFUNCTION, SonyCallback);
    curl_easy_setopt(mem->curl_handle, CURLOPT_WRITEDATA, mem);
    res = curl_easy_perform(mem->curl_handle);
    curl_easy_cleanup(mem->curl_handle);
    curl_global_cleanup();

    // if we get an error from curl_easy_perform
    if ((res != CURLE_OK) && (!mem->stop)) {
        size_t len = strlen(errbuf);
        fprintf(stderr, "\nlibcurl: (%d) ", res);
        if (len)
            fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
        else
            fprintf(stderr, "%s\n", curl_easy_strerror(res));
    }

    // make gcc happy
    return 0;
}

void LoadJPEG(const unsigned char * imgdata, JpegDec_t* jpeg_dec, size_t jpeg_size) {
  struct jpeg_decompress_struct info;
  struct jpeg_error_mgr err;

  info.err = jpeg_std_error(&err);
  jpeg_create_decompress(&info);
  jpeg_mem_src(&info, imgdata, jpeg_size);
  jpeg_read_header(&info, TRUE);
  info.do_fancy_upsampling = FALSE;
  // Free previous jpeg_dec memory (if any)
  free(jpeg_dec->data);

  jpeg_start_decompress(&info);
  jpeg_dec->x = info.output_width;
  jpeg_dec->y = info.output_height;
  jpeg_dec->channels = info.num_components;
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
