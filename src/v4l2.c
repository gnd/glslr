/* *************************************************************************
* This frankenstein was ripped out and glued together from glutcam by George Koharchik:
* https://www.linuxjournal.com/content/image-processing-opengl-and-shaders
*
* Many thanks for this article & code !!
*
* ************************************************************************* */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h> /* stat, open */
#include <sys/types.h>
#include <fcntl.h> /* open   */
#include <errno.h> /* EINTR  */
#include <sys/mman.h> /* mmap  */
#include <sys/ioctl.h>
#include <sys/time.h> /*  24-Jan-08 -gpk  */
#include <stdlib.h>
#include <asm/types.h>          /* for videodev2.h */
#include <linux/videodev2.h>

#include "v4l2.h"
#include "v4l2_controls.h"

#define ERRSTRINGLEN 127

/* DATA_TIMEOUT_INTERVAL - number of microseconds to wait before   */
/* declaring no data available from the input device. tune this    */
/* to the expected speed of your camera. eg:  */
/* 1000000.0 /60 Hz is 16666 usec  */
/* 1000000.0 /30 HZ is 33333 usec */
/* 1000000.0 /15 Hz is 66666 usec */
/* if data is available _before_ this timeout elapses you'll get  */
/*    it when it becomes available.    */

#define DATA_TIMEOUT_INTERVAL (1000000.0 / 60.0)


static int _xioctl(int fd, int request, void * arg) {
    int r;
    do r = ioctl(fd, request, arg);
    while (-1 == r && EINTR == errno);
    return r;
}


void init_device_and_buffers(char * devicename, Sourceparams_t * sourceparams, Videocapabilities_t * capabilities) {

    int fd, buffersize;

    fd = _verify_and_open_device(devicename);

    sourceparams->source = LIVESOURCE;
    sourceparams->fd = fd;
    sourceparams->encoding = LUMA;
    sourceparams->image_width = 640;
    sourceparams->image_height = 480;
    sourceparams->captured.start = NULL;
    buffersize = _compute_bytes_per_frame(640, 480);
    sourceparams->captured.length = buffersize;

    _get_device_capabilities(devicename, fd, capabilities);
    sourceparams->iomethod = IO_METHOD_MMAP;        // select_io_method
    _try_reset_crop_scale(sourceparams);            // set_device_capture_parms
    _set_image_size_and_format(sourceparams);       // set_device_capture_parms
    _request_and_mmap_io_buffers(sourceparams);     // set_io_method, default to MMAP for now
    /* point captured.start at an empty buffer that we can draw until we get data  */
    sourceparams->captured.start = sourceparams->buffers[0].start;
}


int _verify_and_open_device(char * devicename) {
    int fd;
    struct stat buff;
    char errstring[ERRSTRINGLEN];

    if (-1 == stat(devicename, &buff)) {
        sprintf(errstring, "Error: can't 'stat' given device file '%s'\n", devicename);
        perror(errstring);
        fd = -1;
    } else if (!S_ISCHR (buff.st_mode)) {
        fprintf (stderr, "%s is not a character device\n", devicename);
        fd = -1;
    } else {
        fd = open(devicename,  O_RDWR /* required */ | O_NONBLOCK, 0);
        if (-1 == fd)	{
            sprintf(errstring, "Error: can't 'open' given device file '%s'\n", devicename);
            perror(errstring);
        }
    }
    return(fd);
}


void _get_device_capabilities(char * devicename, int device_fd,  Videocapabilities_t * capabilities) {
    memset(capabilities, 0, sizeof(*capabilities));
    _xioctl(device_fd, VIDIOC_QUERYCAP, &(capabilities->capture));

    fprintf(stderr, "Info: '%s' connects to %s using the %s driver\n", devicename, capabilities->capture.card, capabilities->capture.driver);
    _describe_capture_capabilities("Device has the following capabilities\n", &capabilities->capture);
    describe_device_controls("Device has the following controls available\n", devicename, device_fd);
    _collect_supported_image_formats(device_fd, capabilities);

    if (1 == capabilities->supports_yuv422) {
        fprintf(stderr, "device supports -e YUV422\n");
    }
}


void _collect_supported_image_formats(int device_fd,  Videocapabilities_t * capabilities) {
    int retval, indx;
    struct v4l2_fmtdesc format;
    char labelstring[ERRSTRINGLEN];
    indx = 0;

    fprintf(stderr, "Source supplies the following formats:\n");

    do {
        memset(&format, 0, sizeof(format));
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.index = indx;
        retval = _xioctl(device_fd, VIDIOC_ENUM_FMT, &format);

        if (0 == retval) {
            fprintf(stderr, "[%d] %s \n", indx, format.description);
            #ifdef VIDIOC_ENUM_FRAMESIZES
                sprintf(labelstring, "   For %s source offers the following sizes:\n",
                format.description);
                print_supported_framesizes(device_fd, format.pixelformat, labelstring);
            #endif /* VIDIOC_ENUM_FRAMESIZES  */
            if (V4L2_PIX_FMT_YUV420 == format.pixelformat) {
                capabilities->supports_yuv420 = 1;
            } else if (V4L2_PIX_FMT_YUYV == format.pixelformat) {
                capabilities->supports_yuv422  = 1;
            } else if (V4L2_PIX_FMT_GREY == format.pixelformat) {
                capabilities->supports_greyscale   = 1;
            } else if (V4L2_PIX_FMT_RGB24 == format.pixelformat) {
                capabilities->supports_rgb = 1;
            }
            indx = indx + 1;
        }
  } while (0 == retval);
}


void print_supported_framesizes(int device_fd, __u32 pixel_format, char * label) {
    struct v4l2_frmsizeenum sizes;
    int retval, indx, found_a_size;
    indx = 0;
    found_a_size = 0;

    fprintf(stderr, "%s:\n", label);
    do {
        memset(&sizes, 0, sizeof(sizes));
        sizes.index = indx;
        sizes.pixel_format = pixel_format;

        retval = _xioctl (device_fd, VIDIOC_ENUM_FRAMESIZES, &sizes);

        if (0 == retval) {
            found_a_size = 1;
            switch (sizes.type) {
                case V4L2_FRMSIZE_TYPE_DISCRETE:
                    fprintf(stderr, "   [%d] %d x %d\n", sizes.index,
                    sizes.discrete.width,
                    sizes.discrete.height);
                    break;
                case V4L2_FRMSIZE_TYPE_CONTINUOUS: /* fallthrough  */
                case V4L2_FRMSIZE_TYPE_STEPWISE:
                    fprintf(stderr, "  [%d] %d x %d to %d x %d in %x %d steps\n",
                    sizes.index,
                    sizes.stepwise.min_width,
                    sizes.stepwise.min_height,
                    sizes.stepwise.max_width,
                    sizes.stepwise.max_height,
                    sizes.stepwise.step_width,
                    sizes.stepwise.step_height
                    );
                    break;
            default:
                fprintf(stderr, "Error: VIDIOC_ENUM_FRAMESIZES gave unknown type %d to %s", sizes.type, __FUNCTION__);
                fprintf(stderr, "  fix that and recompile\n");
                abort();
                break;
            }
            indx = indx + 1;
        } else {
            /* VIDIOC_ENUM_FRAMESIZES returns -1 and sets errno to EINVAL  */
            /* when you've run out of sizes. so only tell the user we   */
            /* have an error if we didn't find _any_ sizes to report  */

            if (0 == found_a_size) {
                perror("  Warning: can't get size information\n");
            }
        }
    } while (0 == retval);
}


void _describe_capture_capabilities(const char *message, struct v4l2_capability * cap) {
    fprintf(stderr, "%s\n", message);
    fprintf(stderr, "Device: '%s' Driver: '%s'\n", cap->card, cap->driver);
    if (V4L2_CAP_VIDEO_CAPTURE & cap->capabilities) {
        fprintf(stderr, "Device supports video capture.\n");
    } else {
        fprintf(stderr, "Device does NOT support video capture.\n");
    }
    if (V4L2_CAP_READWRITE & cap->capabilities) {
        fprintf(stderr, "Device can supply data by read\n");
    } else {
        fprintf(stderr, "Device can NOT supply data by read\n");
    }
    if (V4L2_CAP_STREAMING & cap->capabilities) {
        fprintf(stderr, "Device supports streaming I/O\n");
    } else {
        fprintf(stderr, "Device does NOT support streaming I/O\n");
    }
    if (V4L2_CAP_ASYNCIO  & cap->capabilities) {
        fprintf(stderr, "Device supports asynchronous I/O\n");
    } else {
        fprintf(stderr, "Device does NOT support asynchronous I/O\n");
    }
}


int _compute_bytes_per_frame(int image_width, int image_height) {
    return (image_width * image_height * 2);
}


void _try_reset_crop_scale(Sourceparams_t * sourceparams) {
    int status;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    char errstring[ERRSTRINGLEN];

    memset(&cropcap, 0, sizeof(cropcap));

    /* set crop/scale to show capture whole picture  */
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* get the area for the whole picture in cropcap.defrect  */
    status = _xioctl (sourceparams->fd, VIDIOC_CROPCAP, &cropcap);

    if (0 ==  status) {
        /* set the area to that whole picture  */
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == _xioctl (sourceparams->fd, VIDIOC_S_CROP, &crop)) {
            switch (errno) {
                case EINVAL:
                    sprintf(errstring, "Warning: VIDIOC_S_CROP cropping not supported\n");
                    /* Cropping not supported. */
                    break;
                default:
                    /* Errors ignored. */
                    sprintf(errstring, "Warning: ignoring VIDIOC_S_CROP error\n");
                    break;
            }
            perror(errstring);
        }
    } else {
        /* Errors ignored. */
        perror("Warning: ignoring error when trying to retrieve crop area\n");
    }
}


int _set_image_size_and_format(Sourceparams_t * sourceparams) {

    struct v4l2_format format;
    int retval;
    char errstring[ERRSTRINGLEN];
    unsigned int requested_height, requested_width;
    unsigned int supplied_height, supplied_width;

    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    /* first get the current format, then change the parts we want  */
    /* to change...  */
    retval = _xioctl (sourceparams->fd, VIDIOC_G_FMT, &format);

    if (-1 == retval) {
        sprintf(errstring, "Fatal error trying to GET format\n");
        perror(errstring);
    } else {
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        format.fmt.pix.width  = sourceparams->image_width;
        format.fmt.pix.height  = sourceparams->image_height;
        format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;

        /* V4L2 spec says interlaced is typical, so let's do that.  */
        /* format.fmt.pix.field = V4L2_FIELD_INTERLACED; */
        /* handle interlace another day; just do progressive scan  */
        /* format.fmt.pix.field = V4L2_FIELD_NONE; */

        retval = _xioctl (sourceparams->fd, VIDIOC_S_FMT, &format);
        if (-1 == retval)	{
            sprintf(errstring, "Fatal error trying to set format %s: format not supported\n", "YUV422");
            perror(errstring);
        } else {
            /* we got an answer back; we asked for a height and width  */
            /* and pixel format; the driver returns what height and   */
            /* width it's going to supply; tell the user if the       */
            /* supplied height and width are not what was requested.  */
            requested_height = sourceparams->image_height;
            requested_width = sourceparams->image_width;
            supplied_height = format.fmt.pix.height;
            supplied_width = format.fmt.pix.width;

            if ((requested_height != supplied_height) || (requested_width != supplied_width)) {
                fprintf(stderr, "Warning: program requested size %d x %d;", sourceparams->image_width, sourceparams->image_height);
                fprintf(stderr, " source offers %d x %d", format.fmt.pix.width, format.fmt.pix.height);
                fprintf(stderr, "Adjusting to %d x %d...\n", format.fmt.pix.width, format.fmt.pix.height);
                sourceparams->image_width = format.fmt.pix.width;
                sourceparams->image_height = format.fmt.pix.height;
            }
        }
    }
    return(retval);
}


int _request_and_mmap_io_buffers(Sourceparams_t * sourceparams) {

    int retval, buffercount;
    /* find out how many buffers are available for mmap access  */
    buffercount = _request_video_buffer_access(sourceparams->fd, V4L2_MEMORY_MMAP);

    if (-1 == buffercount) {
        retval = -1; /* error  */
    } else {
        /* ioctl succeeds: let's see how many buffers we can work with  */
        /* we need at least two.  */
        if (2 > buffercount) {
            fprintf(stderr, "Error: couldn't get enough video buffers from");
            fprintf(stderr, "  the video device. Requested %d, would have", MAX_VIDEO_BUFFERS);
            fprintf(stderr, "settled for 2, got %d\n", buffercount);
            retval = -1; /* error  */
        } else {
            /* we got at least two buffers; call mmap so we can  */
            /* get access to them from user space.   */
            sourceparams->buffercount = buffercount;
            retval = _mmap_io_buffers(sourceparams);

            /* now sourceparams->buffers[0..buffercount -1] point  */
            /* to the device's video buffers, so we can get video  */
            /* data from them.   */
        }
    }
    return(retval);
}


int _request_video_buffer_access(int device_fd, enum v4l2_memory memory) {

    struct v4l2_requestbuffers request;
    int status, retval;
    const char * mmap_error =  "Error: video device doesn't support memory mapping\n";
    const char * userptr_error= "Error: video device doesn't support user pointer I/O\n";
    const char * errstring;

    memset(&request, 0, sizeof(request));
    request.count = MAX_VIDEO_BUFFERS;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = memory;

    status = _xioctl (device_fd, VIDIOC_REQBUFS, &request);

    if (-1 == status) {
        /* assign errstring to the error message we can use if errno is  */
        /* EINVAL  */
        if (V4L2_MEMORY_MMAP == memory) {
            errstring = mmap_error;
        } else {
            errstring = userptr_error;
        }
        /* okay, print an error message  */
        if (EINVAL == errno) {
            fprintf(stderr, "%s\n", errstring);
        } else {
            perror("Error trying to request video buffers from device\n");
        }
            retval = -1; /* error return  */
    } else {
        retval = request.count; /* the number of buffers available  */
    }
    return(retval);
}


int _mmap_io_buffers(Sourceparams_t * sourceparams) {

    int i, status, retval;
    struct v4l2_buffer buf;
    void * mmapped_buffer;

    status = 0;
    retval = 0;

    for (i = 0; (i < sourceparams->buffercount) && (0 == status); i++) {
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        status =  _xioctl(sourceparams->fd, VIDIOC_QUERYBUF, &buf);

        if (-1 == status) {
            perror("Error trying to get buffer info in VIDIOC_QUERYBUF\n");
            retval = -1;
        } else {
            mmapped_buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, sourceparams->fd, buf.m.offset);
            if (MAP_FAILED != mmapped_buffer) {
                sourceparams->buffers[i].length = buf.length;
                sourceparams->buffers[i].start = mmapped_buffer;
            } else {
                perror("Error: can't mmap the video buffer\n");
                retval = -1;
                status = -1;
            }
        }
    }
    return(retval);
}


int _enqueue_mmap_buffers(Sourceparams_t * sourceparams) {
    int i;
    int status;
    struct v4l2_buffer buf;

    status = 0;

    for(i = 0; (i < sourceparams->buffercount) && (0 == status); i++) {
        memset(&buf, 0, sizeof(buf));
        memset(&(buf.timestamp), 0, sizeof(struct timeval));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = (unsigned int)i;
        status =   _xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);
    }
    if (-1 == status) {
        perror("Error enqueueing mmap-ed buffers with VIDIOC_QBUF\n");
    }
    return(status);
}


int _start_streaming(Sourceparams_t * sourceparams) {
    enum v4l2_buf_type type;
    int status;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = _xioctl(sourceparams->fd, VIDIOC_STREAMON, &type);

    if (-1 == status) {
        perror("Error starting streaming with VIDIOC_STREAMON\n");
    }
    return(status);
}


int _stop_streaming(Sourceparams_t * sourceparams) {
    enum v4l2_buf_type type;
    int status;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    status = _xioctl(sourceparams->fd, VIDIOC_STREAMOFF, &type);

    if (-1 == status) {
        perror("Error starting streaming with VIDIOC_STREAMOFF\n");
    }
    return(status);
}


void capture_video_frame(Sourceparams_t * sourceparams, int * nbytesp) {
    //void * retval;
    //retval = next_device_frame(sourceparams, nbytesp);
    //return (retval);
    next_device_frame(sourceparams, nbytesp);
}


void next_device_frame(Sourceparams_t * sourceparams, int * nbytesp) {
    int nbytes;
    //void *datap;
    int data_ready;
    data_ready = wait_for_input(sourceparams->fd , DATA_TIMEOUT_INTERVAL);

    if (-1 == data_ready) {
        /* we had an error waiting on data  */
        //datap = NULL;
        *nbytesp = -1;
    } else if (0 == data_ready) {
        /* no data available in timeout interval  */
        //datap = NULL;
        *nbytesp = 0;
    } else {
        // ONLY MMAP SUPPORTED RIGHT NOW - see the original glutcam next_device_frame for reference
        nbytes = harvest_mmap_device_buffer(sourceparams);
        if (0 < nbytes) {
            //datap = sourceparams->captured.start;
            *nbytesp = nbytes;
        } else {
            //datap = NULL;
            *nbytesp = nbytes;
	    }
    }
    //return(datap);
}


int harvest_mmap_device_buffer(Sourceparams_t * sourceparams) {
    int retval, status;
    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    status = _xioctl (sourceparams->fd, VIDIOC_DQBUF, &buf);

    retval = -1;

    if (-1 == status) {
        /* error dequeueing the buffer  */
        switch(errno) {
            case EAGAIN:
                /* nonblocking and no data ready  */
                perror("VIDIOC_DQBUF: no data ready");
                retval = 0;
                break;
            case EIO: /* fallthrough  */
                /* transient [?] error  */
                perror("VIDIOC_DQBUF: EIO");
                break;
        default:
            perror("Error dequeueing mmap-ed buffer from device");
            retval = -1;
            break;
        }
    } else {
        /* point captured.start at where the data starts in the  */
        /* memory mapped buffer   */
        sourceparams->captured.start = sourceparams->buffers[buf.index].start;
        /* queue the buffer back up again  */
        status = _xioctl (sourceparams->fd, VIDIOC_QBUF, &buf);
        if (-1 == status) {
            /* error re-queueing the buffer  */
            switch(errno) {
                case EAGAIN:
                    /* nonblocking and no data ready  */
                    perror("VIDIOC_DQBUF: no data ready");
                    retval = 0;
                    break;
                case EIO: /* fallthrough  */
                    /* transient [?] error  */
                    perror("VIDIOC_DQBUF: EIO");
                    break;
            default:
                perror("Error dequeueing mmap-ed buffer from device");
                retval = -1;
                break;
            }
        } else {
            retval = sourceparams->captured.length;
        }
    }
    return(retval);
}


int wait_for_input(int fd, int useconds) {
    fd_set readfds;
    int select_result, retval;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    timeout.tv_sec = 0;
    timeout.tv_usec = useconds;

    select_result = select(fd + 1, &readfds, NULL, NULL, &timeout);

    if (-1 == select_result) {
        /* error return from select  */
        perror("Error trying to select looking for input");
        retval = -1; /* error  */
    } else if (0 < select_result) {
        /* we have data  */
        retval = 1;
    } else {
        /* we have a timeout  */
        retval = 0;
    }
    return(retval);
}
