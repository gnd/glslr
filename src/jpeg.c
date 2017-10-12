// taken from https://www.opengl.org/discussion_boards/showthread.php/157667-How-to-load-JPEG-file-for-OpenGL

#include <jpeglib.h>
#include <jerror.h>
//if the jpeglib stuff isnt after I think stdlib then it wont work just put it at the end
unsigned long x;
unsigned long y;
unsigned short int bpp; //bits per pixels   unsigned short int
BYTE* data;             //the data of the image
UINT ID;                //the id ogl gives it
unsigned long size;     //length of the file
int channels;      //the channels of the image 3 = RGA 4 = RGBA
GLuint type;    


bool LoadJPEG(char *imgdata, bool fast = true) {
  FILE* file = fopen(FileName, "rb");  //open the file
  struct jpeg_decompress_struct info;  //the jpeg decompress info
  struct jpeg_error_mgr err;           //the error handler
 
  info.err = jpeg_std_error(&err);     //tell the jpeg decompression handler to send the errors to err
  jpeg_create_decompress(&info);       //sets info to all the default stuff
  jpeg_mem_src(&info, &imgdata, jpeg_size)
  jpeg_read_header(&info, TRUE);   //tell it to start reading it

  if(fast) {
    info.do_fancy_upsampling = FALSE;
  }
 
  jpeg_start_decompress(&info);    //decompress the file
 
  x = info.output_width;
  y = info.output_height;
  channels = info.num_components;
 
  type = GL_RGB;
 
  if(channels == 4) {
    type = GL_RGBA;
  }
 
  bpp = channels * 8;
  size = x * y * 3;
  data = new BYTE[size]; 
  BYTE* p1 = data;
  BYTE** p2 = &p1;
  int numlines = 0;
 
  while(info.output_scanline < info.output_height) {
    numlines = jpeg_read_scanlines(&info, p2, 1);
    *p2 += numlines * 3 * info.output_width;
  }
  jpeg_finish_decompress(&info); 
  fclose(file);                  
  
  return true;
}