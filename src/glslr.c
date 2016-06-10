/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

/* TODO
+- socket into separate file
+- fix tcp receive
+- reactivate mouse input
+*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <errno.h>

#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "config.h"
#include "base.h"
#include "glslr.h"
#include "graphics.h"

/* pdreceive includes */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#define SOCKET_ERROR -1


#define DEBUG 1

#define MAX_SOURCE_BUF (1024*64)
#define MOUSE_DEVICE_PATH "/dev/input/event0"

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <  (b)) ? (a) : (b))
#define CLAMP(min, x, max) MIN(MAX(min, x), max)

typedef struct {
    const char *path;
    time_t last_modify_time;
} SourceObject;


struct PJContext_ {
    Graphics *graphics;
    Graphics_LAYOUT layout_backup;
    int is_fullscreen;
    int use_backbuffer;
    int use_tcp;
    int use_net;
    int port;
    struct {
        int x, y;
        int fd;
    } mouse;
    netin_val *net_input_val;
    int net_params;
    double time_origin;
    unsigned int frame;         /* TODO: move to graphics */
    struct {
        int debug;
        int render_time;
    } verbose;
    struct {
        int numer;
        int denom;
    } scaling;
};

typedef struct _fdpoll
{
    int fdp_fd;
    char *fdp_outbuf;/*output message buffer*/
    int fdp_outlen;     /*length of output message*/
    int fdp_discard;/*buffer overflow: output message is incomplete, discard it*/
    int fdp_gotsemi;/*last char from input was a semicolon*/
} t_fdpoll;

static int nfdpoll;
static t_fdpoll *fdpoll;
static int maxfd;
static int sockfd;
static int protocol;

static void sockerror(const char *s);
static void x_closesocket(int fd);
static void dopoll(PJContext *pj);
#define BUFSIZE 8192


static void addport(int fd)
{
    t_fdpoll *fp;
    fdpoll = (t_fdpoll *)realloc(fdpoll,
        (nfdpoll+1) * sizeof(t_fdpoll));
    fp = fdpoll + nfdpoll;
    fp->fdp_fd = fd;
    nfdpoll++;
    if (fd >= maxfd) maxfd = fd + 1;
    fp->fdp_outlen = fp->fdp_discard = fp->fdp_gotsemi = 0;
    if (!(fp->fdp_outbuf = (char*) malloc(BUFSIZE)))
    {
        fprintf(stderr, "out of memory");
        exit(1);
    }
    printf("number_connected %d;\n", nfdpoll);
}


static void rmport(t_fdpoll *x)
{
    int i;
    t_fdpoll *fp;
    for (i = nfdpoll, fp = fdpoll; i--; fp++)
    {
        if (fp == x)
        {
            x_closesocket(fp->fdp_fd);
            free(fp->fdp_outbuf);
            while (i--)
            {
                fp[0] = fp[1];
                fp++;
            }
            fdpoll = (t_fdpoll *)realloc(fdpoll,
                (nfdpoll-1) * sizeof(t_fdpoll));
            nfdpoll--;
            printf("number_connected %d;\n", nfdpoll);
            return;
        }
    }
    fprintf(stderr, "warning: item removed from poll list but not found");
}


static void doconnect(void)
{
    int fd = accept(sockfd, 0, 0);
    if (fd < 0)
        perror("accept");
    else addport(fd);
}


static void udpread(PJContext *pj)
{
    char buf[BUFSIZE];
    ssize_t len;
    int err;
    len = 1;

    while (len > 0) {
        errno = 0;
        len = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT);
        if (len > 0) {
            udpmakeoutput(buf, pj);
        }
        err = errno;
        errno = 0;
        if (err != 0) {
            /* non-blocking returns -1 and sets errno to EAGAIN or EWOULDBLOCK. */
            if (err == EWOULDBLOCK || err == EAGAIN) {
                /* nothing to read .. */
                break;
            } else {
                sockerror("read(udp)");
                x_closesocket(sockfd);
                exit(1);
            }
    	}
    }
}


void udpmakeoutput(char *buf, PJContext *pj) {

    char line[BUFSIZE];
    char *p, *val;
    netin_val *curr=NULL;
    int i = 0;

    while ((buf[i] != ';') && (i < BUFSIZE)) {
        line[i] = buf[i];
        i++;
    }

    p = line;

    i = 0;
    curr = pj->net_input_val;
    while (((val = strsep(&p, " ")) != NULL) && (i < pj->net_params)) {
	curr->val = strtof(val,NULL);
        curr = curr->next;
	i++;
    }
}


static int tcpmakeoutput(t_fdpoll *x, char *inbuf, int len)
{
    // CURRENTLY BROKEN
    int i;
    int outlen = x->fdp_outlen;
    char *outbuf = x->fdp_outbuf;

    for (i = 2 ; i < len ; i++)
    {
        char c = inbuf[i];
        if((c != '\n') || (!x->fdp_gotsemi))
            outbuf[outlen++] = c;
        x->fdp_gotsemi = 0;
	//output buffer overflow; reserve 1 for '\n' 
        if (outlen >= (BUFSIZE-1))
        {
            fprintf(stderr, "message too long; discarding\n");
            outlen = 0;
            x->fdp_discard = 1;
        }
        if (c == ';')
        {
            if (!x->fdp_discard)
            {
	    // fix
            }
            outlen = 0;
            x->fdp_discard = 0;
            x->fdp_gotsemi = 1;
        }
    }

    x->fdp_outlen = outlen;
    return (0);
}


static void tcpread(t_fdpoll *x)
{
    int  ret;
    char inbuf[BUFSIZE];

    if ((ret = read(x->fdp_fd, inbuf, BUFSIZE)) < 0) { 
        if (errno != EWOULDBLOCK) 
            sockerror("read error on socket"); 
    }
    else if (ret == 0)
        rmport(x);
    else tcpmakeoutput(x, inbuf, ret);
}

static void sockerror(const char *s)
{
    int err = errno;
    fprintf(stderr, "%s: %s (%d)\n", s, strerror(err), err);
}

static void x_closesocket(int fd)
{
    close(fd);
}

static void dopoll(PJContext *pj)
{
    t_fdpoll *fp;
    fd_set readset;
    struct timeval tv;
    int i;  

    FD_ZERO(&readset);
    FD_SET(fileno(stdin), &readset);
    // no mouse FD_SET(pj->mouse.fd, &readset);
    if (pj->use_tcp) {
    	for (fp = fdpoll, i = nfdpoll; i--; fp++)
        	FD_SET(fp->fdp_fd, &readset);
    } else {
        FD_SET(sockfd, &readset);
    }

    // timeout on socket - wait 1ms
    tv.tv_sec = 0;
    tv.tv_usec = 100;

    if (select(maxfd+1, &readset, NULL, NULL, &tv) < 0)
    {   
        perror("select");
        exit(1);
    } else {
        if (pj->use_tcp) {
	    for (i = 0; i < nfdpoll; i++)
            if (FD_ISSET(fdpoll[i].fdp_fd, &readset))
                tcpread(&fdpoll[i]);
            if (FD_ISSET(sockfd, &readset))
                doconnect();
        } else {
            if (FD_ISSET(sockfd, &readset))
            {   
                udpread(pj);
            }
        }
    }
}

#define PJDebug(pj, printf_arg) ((pj)->verbose.debug ? (printf printf_arg) : 0)


static double GetCurrentTimeInMilliSecond(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static int GetLastFileModifyTime(const char *file_path, time_t *out_mod_time)
{
    struct stat sb;
    if (stat(file_path, &sb) == -1) {
        return 1;
    }
    *out_mod_time = sb.st_mtime;
    return 0;
}

/* SourceObject */
static SourceObject *SourceObject_Create(const char *path)
{
    SourceObject *so;
    so = malloc(sizeof(*so));
    so->path = path;
    so->last_modify_time = 0;
    return so;
}

static void SourceObject_Delete(void *so)
{
    free(so);
}

/* PJContext */
int PJContext_Construct(PJContext *pj)
{
    int scaling_numer, scaling_denom;
    scaling_numer = 1;
    scaling_denom = 2;
    pj->graphics = Graphics_Create(Graphics_LAYOUT_FULLSCREEN,
                                   scaling_numer, scaling_denom);
    if (!pj->graphics) {
        fprintf(stderr, "Graphics Initialize failed:\r\n");
        return 2;
    }

    pj->is_fullscreen = 0;
    pj->use_backbuffer = 0;
    pj->use_tcp = 0;
    pj->use_net = 0;
    pj->port = 6666;
    pj->mouse.x = 0;
    pj->mouse.y = 0;
    pj->mouse.fd = open(MOUSE_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
    pj->net_input_val = NULL;
    pj->net_params = 0;
    pj->time_origin = GetCurrentTimeInMilliSecond();
    pj->frame = 0;
    pj->verbose.render_time = 0;
    pj->verbose.debug = 0;
    pj->scaling.numer = scaling_numer;
    pj->scaling.denom = scaling_denom;
    return 0;
}

void PJContext_Listen(int use_tcp, int port)
{
    int portno = port;
    struct sockaddr_in server;
    
    if (use_tcp) {
        protocol = SOCK_STREAM;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {   
            sockerror("socket()");
            exit(1);
        }
        int val = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
        maxfd = sockfd + 1;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons((unsigned short)portno);
        if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
        {   
            sockerror("bind");
            x_closesocket(sockfd);
            //return (0);
        }
        if (listen(sockfd, 5) < 0)
        {
            sockerror("listen");
            x_closesocket(sockfd);
            exit(1);
        }

    // UDP
    } else {
        protocol = SOCK_DGRAM;
        sockfd = socket(AF_INET, protocol, 0);
	if (sockfd < 0)
        {   
            sockerror("socket()");
            exit(1);
        }
	    maxfd = sockfd + 1;
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons((unsigned short)portno);
        if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0)
        {   
            sockerror("bind");
            x_closesocket(sockfd);
            //return (0);
        }
    }
}

void PJContext_Destruct(PJContext *pj)
{
    int i;
    RenderLayer *layer;

    if (pj->mouse.fd >= 0) {
        close(pj->mouse.fd);
    }
    for (i = 0; (layer = Graphics_GetRenderLayer(pj->graphics, i)) != NULL; i++) {
        SourceObject_Delete(RenderLayer_GetAux(layer));
    }
    Graphics_Delete(pj->graphics);
}


static int PJContext_SwitchBackbuffer(PJContext *pj)
{
    pj->use_backbuffer ^= 1;
    Graphics_SetBackbuffer(pj->graphics, pj->use_backbuffer);
    return Graphics_ApplyOffscreenChange(pj->graphics);
}

static int PJContext_ChangeScaling(PJContext *pj, int add)
{
    pj->scaling.denom += add;
    if (pj->scaling.denom <= 0) {
        pj->scaling.denom = 1;
    }
    if (pj->scaling.denom >= 16) {
        pj->scaling.denom = 16;
    }
    Graphics_SetWindowScaling(pj->graphics, pj->scaling.numer, pj->scaling.denom);
    Graphics_ApplyWindowScalingChange(pj->graphics);
    {
        int width, height;
        Graphics_GetSourceSize(pj->graphics, &width, &height);
        printf("offscreen size = %dx%d px, scaling = %d/%d\r\n",
               width, height, pj->scaling.numer, pj->scaling.denom);
    }
    return 0;
}

static int PJContext_ReloadAndRebuildShadersIfNeed(PJContext *pj)
{
    int i;
    RenderLayer *layer;

    /* PJDebug(pj, ("PJContext_ReloadAndRebuildShadersIfNeed\r\n")); */
    for (i = 0; (layer = Graphics_GetRenderLayer(pj->graphics, i)) != NULL; i++) {
        time_t t;
        SourceObject *so;
        so = RenderLayer_GetAux(layer);
        if (GetLastFileModifyTime(so->path, &t)) {
            fprintf(stderr, "file open failed: %s\r\n", so->path);
            continue;
        }
        if (so->last_modify_time != t) {
            FILE *fp;
            fp = fopen(so->path, "r");
            if (fp == NULL) {
                fprintf(stderr, "file open failed: %s\r\n", so->path);
            } else {
                size_t len;
                char code[MAX_SOURCE_BUF]; /* hmm.. */
                errno = 0;
                len = fread(code, 1, sizeof(code), fp);
                /* TODO: handle errno */
                if (ferror(fp) != 0) {
                    PJDebug(pj, ("ferror = %d\r\n", ferror(fp)));
                }
                if (errno != 0) {
                    PJDebug(pj, ("errno = %d\r\n", errno));
                }
                PJDebug(pj, ("update: %s\r\n", so->path));
                RenderLayer_UpdateShaderSource(layer, code, (int)len);
                so->last_modify_time = t;
                Graphics_BuildRenderLayer(pj->graphics, i);
            }
        }
    }
    return 0;
}

static void PJContext_UpdateMousePosition(PJContext *pj)
{
    int i;
    const int max_zap_event = 16;

    if (pj->mouse.fd < 0) {
        return;
    }

    for (i = 0; i < max_zap_event; i++) {
        struct input_event ev;
        ssize_t len;
        int err;
        errno = 0;
	len = read(pj->mouse.fd, &ev, sizeof(ev));
	err = errno;
        errno = 0;
	if (len != sizeof(ev)) {
            /* no more data */
            break;
        }
        if (err != 0) {
            if (err == EWOULDBLOCK || err == EAGAIN) {
                /* ok... try again next time */
                break;
            } else {
                printf("error on mouse-read: code %d(%s)\r\n", err, strerror(err));
                return;
            }
        }
        if (ev.type == EV_REL) { /* relative-move event */
            switch (ev.code) {
            case REL_X:
                pj->mouse.x += (int)ev.value;
                break;
            case REL_Y:
                pj->mouse.y += -(int)ev.value;
                break;
            default:
                break;
            }
        }
    }

    {
        int width, height;
        /* fix mouse position */
        Graphics_GetWindowSize(pj->graphics, &width, &height);
	pj->mouse.x = CLAMP(0, pj->mouse.x, width);
        pj->mouse.y = CLAMP(0, pj->mouse.y, height);
    }
}

static void PJContext_SetUniforms(PJContext *pj)
{
    double t;
    double mouse_x, mouse_y;
    int width, height;
    t = GetCurrentTimeInMilliSecond() - pj->time_origin;
    Graphics_GetWindowSize(pj->graphics, &width, &height);
    mouse_x = (double)pj->mouse.x / width;
    mouse_y = (double)pj->mouse.y / height;

    Graphics_SetUniforms(pj->graphics, t / 1000.0, 
						 pj->net_input_val,
                         mouse_x, mouse_y, drand48(), drand48());
}

static void PJContext_Render(PJContext *pj)
{
    if (pj->verbose.render_time) {
        double t, ms;
        t = GetCurrentTimeInMilliSecond();
        Graphics_Render(pj->graphics);
        ms = GetCurrentTimeInMilliSecond() - t;
        printf("render time: %.1f ms (%.0f fps)    \r", ms, 1000.0 / ms);
    } else {
        Graphics_Render(pj->graphics);
    }
}

static void PJContext_AdvanceFrame(PJContext *pj)
{
    pj->frame += 1;
}

static int PJContext_Update(PJContext *pj)
{
	glfwPollEvents();
    if (PJContext_ReloadAndRebuildShadersIfNeed(pj)) {
        return 1;
    }
    if (pj->use_net) {
        dopoll(pj);
    }
    /*PJContext_UpdateMousePosition(pj);*/
    PJContext_SetUniforms(pj);
    PJContext_Render(pj);
    PJContext_AdvanceFrame(pj);
    return 0;
}

static void PrintHelp(void)
{
    printf("Key:\r\n");
    printf("  t        FPS printing\r\n");
    printf("  [ or ]   offscreen scaling\r\n");
    printf("  b        backbuffer ON/OFF\r\n");
    printf("  q        exit\r\n");
}

#if 0
static int PJContext_HandleKeyboadEvent(PJContext *pj)
{
    /* TODO */
    return 0;
}
#endif

static int PJContext_PrepareMainLoop(PJContext *pj)
{
    return Graphics_AllocateOffscreen(pj->graphics);    
}

static void PJContext_MainLoop(PJContext *pj)
{
    for (;;) {
        switch (getchar()) {
        case 'Q':
        case 'q':
        case VEOF:      /* Ctrl+d */
        case VINTR:     /* Ctrl+c */
        case 0x7f:      /* Ctrl+c */
        case 0x03:      /* Ctrl+c */
        case 0x1b:      /* ESC */
            printf("\r\nexit\r\n");
            goto goal;
        case ']':
            PJContext_ChangeScaling(pj, 1);
            break;
        case '[':
            PJContext_ChangeScaling(pj, -1);
            break;
        case 't':
        case 'T':
            pj->verbose.render_time ^= 1;
            printf("\r\n");
            break;
        case 'b':
            PJContext_SwitchBackbuffer(pj);
            printf("backbuffer %s\r\n", pj->use_backbuffer ? "ON": "OFF");
            break;
        case '?':
            PrintHelp();
        default:
            break;
        }
        if (PJContext_Update(pj)) {
            break;
        }
    }
  goal:
    return;
}

static int PJContext_AppendLayer(PJContext *pj, const char *path)
{
    SourceObject *so;
    FILE *fp;
    char code[MAX_SOURCE_BUF];
    size_t len;
    PJDebug(pj, ("PJContext_AppendLayer: %s\r\n", path));
    fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "file open failed: %s\r\n", path);
        return 1;
    }
    len = fread(code, 1, sizeof(code), fp);
    fclose(fp);
    so = SourceObject_Create(path);
    Graphics_AppendRenderLayer(pj->graphics, code, (int)len, (void *)so);
    return 0;
}

int PJContext_ParseArgs(PJContext *pj, int argc, const char *argv[])
{
    int i;
    int layer;
    Graphics *g;

    g = pj->graphics;
    layer = 0;
    for (i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if (strcmp(arg, "--debug") == 0) {
            pj->verbose.debug = 1;
        } else if (strcmp(arg, "--RGB888") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB888);
        } else if (strcmp(arg, "--RGBA8888") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGBA8888);
        } else if (strcmp(arg, "--RGB565") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB565);
	} else if (strcmp(arg, "--RGBA4444") == 0) {
            Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGBA4444);
        } else if (strcmp(arg, "--nearestneighbor") == 0) {
            Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR);
        } else if (strcmp(arg, "--bilinear") == 0) {
            Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_BILINEAR);
        } else if (strcmp(arg, "--wrap-clamp_to_edge") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_CLAMP_TO_EDGE);
        } else if (strcmp(arg, "--wrap-repeat") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_REPEAT);
        } else if (strcmp(arg, "--wrap-mirror_repeat") == 0) {
            Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_MIRRORED_REPEAT);
        } else if (strcmp(arg, "--backbuffer") == 0) {
            pj->use_backbuffer = 1;
        } else if (strcmp(arg, "--net") == 0) {
	    pj->use_net = 1;
        } else if (strcmp(arg, "--tcp") == 0) {
            pj->use_tcp = 1;
        } else if (strcmp(arg, "--port") == 0) {
            pj->port = strtol(argv[i+1], NULL, 10);
            i++;
	// how much parameters to have
        } else if (strcmp(arg, "--params") == 0) {
            pj->net_params=atoi(argv[i+1]);
	    i++;
        } else {
            printf("layer %d: %s\r\n", layer, arg);
            PJContext_AppendLayer(pj, arg);
            layer += 1;
        }
    }
    Graphics_SetBackbuffer(g, pj->use_backbuffer);
   
    // create net_input_val linked list
    netin_val *curr=NULL;
    netin_val *next=NULL; 
    i = 0;
    while (i < pj->net_params) {
        curr=malloc(sizeof(netin_val));
        curr->next = next;
	curr->val = 0;
        next = curr;
	i++;
    }
    pj->net_input_val = next;
    Graphics_SetNetParams(g, pj->net_params);
    Graphics_ApplyOffscreenChange(pj->graphics);
    if (pj->use_net) {
     	PJContext_Listen(pj->use_tcp, pj->port);
    }
    return (layer == 0) ? 1 : 0;
}

int PJContext_HostInitialize(void)
{
    Graphics_HostInitialize();
    return 0;
}

void PJContext_HostDeinitialize(void)
{
    Graphics_HostDeinitialize();
}

size_t PJContext_InstanceSize(void)
{
    return sizeof(PJContext);
}

int PJContext_Main(PJContext *pj)
{
    PJContext_PrepareMainLoop(pj);
    PJContext_MainLoop(pj);
    return EXIT_SUCCESS;
}

