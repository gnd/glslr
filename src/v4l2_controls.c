/* ************************************************************************* 
* This frankenstein was ripped out and glued together from glutcam by George Koharchik:
* https://www.linuxjournal.com/content/image-processing-opengl-and-shaders
* 
* Many thanks for this article & code !!
*
* ************************************************************************* */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include "v4l2_controls.h"

static void enumerate_menu (char * label, int fd,  struct v4l2_queryctrl queryctrl);
void explain_control_type(char * label, struct v4l2_queryctrl queryctrl, int fd);


void describe_device_controls(char * label, char * devicename, int device_fd) {
    struct v4l2_queryctrl queryctrl;
    fprintf(stderr, "%s", label);
    fprintf(stderr, " using device file %s\n", devicename);
    memset (&queryctrl, 0, sizeof (queryctrl));

    for (queryctrl.id = V4L2_CID_BASE; queryctrl.id < V4L2_CID_LASTP1; queryctrl.id++) {
        if (0 == ioctl (device_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                continue;
            } else {
                explain_control_type("", queryctrl, device_fd);
            }
        } else {
            if (errno == EINVAL) { 
                continue;
            }
            perror ("VIDIOC_QUERYCTRL");
            exit (EXIT_FAILURE);
        }
    }
    for (queryctrl.id = V4L2_CID_PRIVATE_BASE; ; queryctrl.id++) {
        if (0 == ioctl (device_fd, VIDIOC_QUERYCTRL, &queryctrl)) {
            if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                continue;
            } else {
                explain_control_type("", queryctrl, device_fd);
            }
        } else {
            if (errno == EINVAL) {
                break;
            }
            perror ("VIDIOC_QUERYCTRL");
        exit (EXIT_FAILURE);
        }
    }
}



static void enumerate_menu (char * label, int fd, struct v4l2_queryctrl queryctrl) {
    struct v4l2_querymenu querymenu;
    fprintf (stderr, "%s:\n", label);
    fprintf (stderr, "  Menu items:\n");
    memset (&querymenu, 0, sizeof (querymenu));
    querymenu.id = queryctrl.id;

    for (querymenu.index = queryctrl.minimum; querymenu.index <= (unsigned int)queryctrl.maximum; querymenu.index++) {
        if (0 == ioctl (fd, VIDIOC_QUERYMENU, &querymenu)) {
            fprintf (stderr, "  %s\n", querymenu.name);
        } else {
            perror ("VIDIOC_QUERYMENU");
            exit (EXIT_FAILURE);
        }
    }
}



void explain_control_type(char * label, struct v4l2_queryctrl queryctrl, int fd) { 
    fprintf(stderr, "%s ", label);
    fprintf (stderr, "Control %s:", queryctrl.name);

    switch (queryctrl.type) {
        case V4L2_CTRL_TYPE_INTEGER:
            /* tell me the range  */
            fprintf (stderr, "integer %d to %d in increments of %d\n", queryctrl.minimum, queryctrl.maximum, queryctrl.step);
            break;
        case V4L2_CTRL_TYPE_BOOLEAN:
            fprintf (stderr, "boolean %d or %d\n", queryctrl.minimum,
            queryctrl.maximum);
            break;
        case V4L2_CTRL_TYPE_MENU:
            enumerate_menu((char *)queryctrl.name, fd, queryctrl);
            break;
        case V4L2_CTRL_TYPE_BUTTON:
            ; /* empty statement  */
            fprintf(stderr, "(button)\n");
            break;
        #ifdef V4L2_CTRL_TYPE_INTEGER64
        case V4L2_CTRL_TYPE_INTEGER64:
            fprintf(stderr, "value is a 64-bit integer\n");
            break;
        #endif /* V4L2_CTRL_TYPE_INTEGER64  */
        #ifdef V4L2_CTRL_TYPE_CTRL_CLASS
        case V4L2_CTRL_TYPE_CTRL_CLASS:
            ;/* empty statement  */
            break;
        #endif /* V4L2_CTRL_TYPE_CTRL_CLASS  */
    default:
        fprintf(stderr, "Warning: unknown control type in %s\n", __FUNCTION__);
        break;
    }
}