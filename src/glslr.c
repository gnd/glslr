/* TODO
+- socket into separate file
+- fix tcp receive
+- reactivate mouse input
+*/
#include "glslr.h"

#define SOCKET_ERROR -1
#define DEBUG 1
#define MAX_SOURCE_BUF (1024*64)
#define MOUSE_DEVICE_PATH "/dev/input/event0"
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <  (b)) ? (a) : (b))
#define CLAMP(min, x, max) MIN(MAX(min, x), max)
#define BUFSIZE 8192
#define GXDebug(gx, printf_arg) ((gx)->verbose.debug ? (printf printf_arg) : 0)

// Some globals
JpegDec_t jpeg_dec;
static int nfdpoll;
static t_fdpoll *fdpoll;
static int maxfd;
static int sockfd;
static int protocol;
pthread_mutex_t video_mutex;

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
	if (!(fp->fdp_outbuf = (char*) malloc(BUFSIZE))) {
		fprintf(stderr, "out of memory");
		exit(1);
	}
	printf("number_connected %d;\n", nfdpoll);
}


static void rmport(t_fdpoll *x)
{
	int i;
	t_fdpoll *fp;
	for (i = nfdpoll, fp = fdpoll; i--; fp++) {
		if (fp == x) {
			x_closesocket(fp->fdp_fd);
			free(fp->fdp_outbuf);
			while (i--) {
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
	fprintf(stderr, "warning: item removed from poll list but not found\n");
}


static void doconnect(void)
{
	int fd = accept(sockfd, 0, 0);
	if (fd < 0)
		perror("accept");
	else addport(fd);
}


static void udpread(Glslr *gx)
{
	char buf[BUFSIZE];
	ssize_t len;
	int err;
	len = 1;

	while (len > 0) {
		errno = 0;
		len = recv(sockfd, buf, BUFSIZE, MSG_DONTWAIT);
		if (len > 0) {
			udpmakeoutput(buf, gx);
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


void udpmakeoutput(char *buf, Glslr *gx)
{

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
	curr = gx->net_input_val;
	while (((val = strsep(&p, " ")) != NULL) && (i < gx->net_params)) {
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

	for (i = 2 ; i < len ; i++) {
		char c = inbuf[i];
		if((c != '\n') || (!x->fdp_gotsemi))
			outbuf[outlen++] = c;
		x->fdp_gotsemi = 0;
		//output buffer overflow; reserve 1 for '\n'
		if (outlen >= (BUFSIZE-1)) {
			fprintf(stderr, "message too long; discarding\n");
			outlen = 0;
			x->fdp_discard = 1;
		}
		if (c == ';') {
			if (!x->fdp_discard) {
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

void tcpread(t_fdpoll *x)
{
	int  ret;
	char inbuf[BUFSIZE];

	if ((ret = read(x->fdp_fd, inbuf, BUFSIZE)) < 0) {
		if (errno != EWOULDBLOCK)
			sockerror("read error on socket");
	} else if (ret == 0)
		rmport(x);
	else tcpmakeoutput(x, inbuf, ret);
}

void sockerror(const char *s)
{
	int err = errno;
	fprintf(stderr, "%s: %s (%d)\n", s, strerror(err), err);
}

void x_closesocket(int fd)
{
	close(fd);
}

void dopoll(Glslr *gx)
{
	t_fdpoll *fp;
	fd_set readset;
	struct timeval tv;
	int i;

	FD_ZERO(&readset);
	FD_SET(fileno(stdin), &readset);
	// no mouse FD_SET(gx->mouse.fd, &readset);
	if (gx->use_tcp) {
		for (fp = fdpoll, i = nfdpoll; i--; fp++)
			FD_SET(fp->fdp_fd, &readset);
	} else {
		FD_SET(sockfd, &readset);
	}

	// timeout on socket - wait 1ms
	tv.tv_sec = 0;
	tv.tv_usec = 100;

	if (select(maxfd+1, &readset, NULL, NULL, &tv) < 0) {
		perror("select");
		exit(1);
	} else {
		if (gx->use_tcp) {
			for (i = 0; i < nfdpoll; i++)
				if (FD_ISSET(fdpoll[i].fdp_fd, &readset))
					tcpread(&fdpoll[i]);
			if (FD_ISSET(sockfd, &readset))
				doconnect();
		} else {
			if (FD_ISSET(sockfd, &readset)) {
				udpread(gx);
			}
		}
	}
}

static double GetCurrentTimeInMilliSecond(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// TODO change this so that also "included" code is checked for changes
static int GetLastFileModifyTime(const char *file_path, time_t *out_mod_time)
{
	struct stat sb;
	if (stat(file_path, &sb) == -1) {
        int errsv = errno;
        if (errsv == ENOENT ){ printf("ENOENT: %s\n", file_path); }
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

/* Glslr */
int Glslr_Construct(Glslr *gx)
{
	int scaling_numer, scaling_denom;
	scaling_numer = 1;
	scaling_denom = 2;
	gx->graphics = Graphics_Create(Graphics_LAYOUT_PRIMARY_RESOLUTION,
	                               scaling_numer, scaling_denom);
	if (!gx->graphics) {
		fprintf(stderr, "Graphics Initialize failed:\n");
		return 2;
	}

	#ifdef VIDEO
    memset(&gx->sourceparams, 0, sizeof(gx->sourceparams));
	#endif

	gx->sony_thread_active = false;
	gx->is_fullscreen = 0;
	gx->use_backbuffer = 0;
    gx->use_video = 0;
    gx->use_sony = 0;
	gx->do_save = 0;
	gx->use_tcp = 0;
	gx->use_net = 0;
	gx->port = 6666;
	gx->mouse.x = 0;
	gx->mouse.y = 0;
	//gx->mouse.fd = open(MOUSE_DEVICE_PATH, O_RDONLY | O_NONBLOCK);
	gx->net_input_val = NULL;
	gx->net_params = 0;
    gx->video_dev_num = 666;
    //gx->video_device = NULL;
	gx->time_origin = GetCurrentTimeInMilliSecond();
	gx->frame = 0;
	gx->verbose.render_time = 0;
	gx->verbose.debug = 0;
	gx->scaling.numer = scaling_numer;
	gx->scaling.denom = scaling_denom;
	gx->save_tga = 0;
	gx->save_dirpath = NULL;
	gx->save_filename = NULL;

	// TODO - check if all below needed
	memset(&gx->mem, 0, sizeof(gx->mem));
	gx->mem.memory = malloc(1);
	gx->mem.size = 0;
	gx->mem.header_found = false;
	gx->mem.size_string = malloc(6);
	gx->mem.jpeg_size = 0;
	gx->mem.stop = false;

	memset(&jpeg_dec, 0, sizeof(jpeg_dec));
	jpeg_dec.x = 0;
	jpeg_dec.y = 0;
	jpeg_dec.bpp = 0;
	jpeg_dec.data = NULL;
	jpeg_dec.size = 0;
	jpeg_dec.channels = 0;

	pthread_mutex_init(&video_mutex, NULL);

	return 0;
}

void Glslr_Listen(int use_tcp, int port)
{
	int portno = port;
	struct sockaddr_in server;

	if (use_tcp) {
		protocol = SOCK_STREAM;
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0) {
			sockerror("socket()");
			exit(1);
		}
		int val = fcntl(sockfd, F_GETFL, 0);
		fcntl(sockfd, F_SETFL, val | O_NONBLOCK);
		maxfd = sockfd + 1;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons((unsigned short)portno);
		if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
			sockerror("bind");
			x_closesocket(sockfd);
			//return (0);
		}
		if (listen(sockfd, 5) < 0) {
			sockerror("listen");
			x_closesocket(sockfd);
			exit(1);
		}

	// UDP
	} else {
		protocol = SOCK_DGRAM;
		sockfd = socket(AF_INET, protocol, 0);
		if (sockfd < 0) {
			sockerror("socket()");
			exit(1);
		}
		maxfd = sockfd + 1;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = htons((unsigned short)portno);
		if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
			sockerror("bind");
			x_closesocket(sockfd);
			//return (0);
		}
	}
}

void Glslr_Destruct(Glslr *gx)
{
	int i;
	RenderLayer *layer;

	if (gx->mouse.fd >= 0) {
		close(gx->mouse.fd);
	}
	for (i = 0; (layer = Graphics_GetRenderLayer(gx->graphics, i)) != NULL; i++) {
		SourceObject_Delete(RenderLayer_GetAux(layer));
	}
	Graphics_Delete(gx->graphics);
    // free the jpeg reading memory
    free(gx->mem.memory);
}

void SetSaveTGA(Glslr *gx)
{
    gx->save_tga = 1;
	Graphics_SetSaveFormat(gx->graphics, "TGA");
}

void SetSaveDir(Glslr *gx, char *dirpath)
{
    gx->save_dirpath = malloc(strlen(dirpath) + 1);
    strcpy(gx->save_dirpath, dirpath);
}

void SetSaveFile(Glslr *gx, char *filename)
{
    gx->save_filename = malloc(strlen(filename) + 1);
    strcpy(gx->save_filename, filename);
}

// TODO - why so IF - make this more concise by setting extension first
void Glslr_SetSaveDestination(Glslr *gx, Graphics *g)
{
    if (gx->save_dirpath == NULL) {
        if (gx->save_filename == NULL) {
			if (gx->save_tga == 1) {
				// using default filename with TGA
	            g->save_name = malloc(strlen("glslr_%05d.tga") + 1);
	            strcpy(g->save_name, "glslr_%05d.tga");
			} else {
				// using default filename with JPEG
	            g->save_name = malloc(strlen("glslr_%05d.jpg") + 1);
	            strcpy(g->save_name, "glslr_%05d.jpg");
			}
        } else {
			if (gx->save_tga == 1) {
            	g->save_name = malloc(strlen(gx->save_filename) + 4 + 1); // .. + strlen(".tga") + 1
            	strcpy(g->save_name, gx->save_filename);
				strcat(g->save_name, ".tga");
			} else {
				g->save_name = malloc(strlen(gx->save_filename) + 4 + 1); // .. + strlen(".jpg") + 1
            	strcpy(g->save_name, gx->save_filename);
				strcat(g->save_name, ".jpg");
			}
        }
    } else {
        if (gx->save_filename == NULL) {
			if (gx->save_tga == 1) {
            	// using default filename with tga
            	g->save_name = malloc(strlen(gx->save_dirpath) + strlen("glslr_%05d.tga") + sizeof(char) + 1);
            	strcpy(g->save_name, gx->save_dirpath);
            	strcat(g->save_name, "/");
            	strcat(g->save_name, "glslr_%05d.tga");
			} else {
				// using default filename with jpg
            	g->save_name = malloc(strlen(gx->save_dirpath) + strlen("glslr_%05d.jpg") + sizeof(char) + 1);
            	strcpy(g->save_name, gx->save_dirpath);
            	strcat(g->save_name, "/");
            	strcat(g->save_name, "glslr_%05d.jpg");
			}
        } else {
			if (gx->save_tga == 1) {
	            g->save_name = malloc(strlen(gx->save_dirpath) + strlen(gx->save_filename) + sizeof(char)*5 + 1);
	            strcpy(g->save_name, gx->save_dirpath);
	            strcat(g->save_name, "/");
	            strcat(g->save_name, gx->save_filename);
				strcat(g->save_name, ".tga");
			} else {
				g->save_name = malloc(strlen(gx->save_dirpath) + strlen(gx->save_filename) + sizeof(char)*5 + 1);
	            strcpy(g->save_name, gx->save_dirpath);
	            strcat(g->save_name, "/");
	            strcat(g->save_name, gx->save_filename);
				strcat(g->save_name, ".jpg");
			}
        }
    }
    printf("Files will be saved to: %s\n", g->save_name);
}


static int Glslr_SwitchBackbuffer(Glslr *gx)
{
	gx->use_backbuffer ^= 1;
	Graphics_SetBackbuffer(gx->graphics, gx->use_backbuffer);
	return Graphics_ApplyOffscreenChange(gx->graphics);
}


static int Glslr_SwitchVideo(Glslr *gx)
{
	gx->use_video ^= 1;
	#ifdef VIDEO
	Graphics_SetVideo(gx->graphics, gx->use_video);
    if (gx->use_video == 1) {
            _enqueue_mmap_buffers(&gx->sourceparams);
            _start_streaming(&gx->sourceparams);
    } else {
        _stop_streaming(&gx->sourceparams);
    }
	return Graphics_ApplyOffscreenChange(gx->graphics);
	#else
	return 0;
	#endif
}


static int Glslr_SwitchSony(Glslr *gx)
{
	gx->use_sony ^= 1;
	Graphics_SetSony(gx->graphics, gx->use_sony);
	return Graphics_ApplyOffscreenChange(gx->graphics);
}


static int Glslr_SwitchSave(Glslr *gx)
{
	gx->do_save ^= 1;
	Graphics_SetSave(gx->graphics, gx->do_save);
	return 0;
}


/* currently not working
static int Glslr_ChangeScaling(Glslr *gx, int add)
{
	gx->scaling.denom += add;
	if (gx->scaling.denom <= 0) {
		gx->scaling.denom = 1;
	}
	if (gx->scaling.denom >= 16) {
		gx->scaling.denom = 16;
	}
	Graphics_SetWindowScaling(gx->graphics, gx->scaling.numer, gx->scaling.denom);
	Graphics_ApplyWindowScalingChange(gx->graphics);
	{
		int width, height;
		Graphics_GetSourceSize(gx->graphics, &width, &height);
		printf("offscreen size = %dx%d px, scaling = %d/%d\n",
		       width, height, gx->scaling.numer, gx->scaling.denom);
	}
	return 0;
} */

static int Glslr_ReloadAndRebuildShadersIfNeed(Glslr *gx)
{
	int i, lines_before, lines_included;
	RenderLayer *layer;

	for (i = 0; (layer = Graphics_GetRenderLayer(gx->graphics, i)) != NULL; i++) {
		time_t t;
		SourceObject *so;
		so = RenderLayer_GetAux(layer);
		if (GetLastFileModifyTime(so->path, &t)) {
			fprintf(stderr, "[1] file open failed: %s\n", so->path);
			continue;
		}
		if (so->last_modify_time != t) {
			FILE *fp;
			fp = fopen(so->path, "r");
			if (fp == NULL) {
				fprintf(stderr, "[2] file open failed: %s\n", so->path);
			} else {
				int len = 0;
				char code[MAX_SOURCE_BUF]; /* hmm.. */
                memset(code,'\0',MAX_SOURCE_BUF);
				errno = 0;
				len = fread(code, 1, sizeof(code), fp);

				/* TODO: handle errno */
				if (ferror(fp) != 0) {
					GXDebug(gx, ("ferror = %d\n", ferror(fp)));
				}
				if (errno != 0) {
					GXDebug(gx, ("errno = %d\n", errno));
				}
                fclose(fp);
				GXDebug(gx, ("update: %s\n", so->path));
                // here comes include extend function
                Glslr_IncludeAdditionalCode(code, &len, &lines_before, &lines_included);
				RenderLayer_UpdateShaderSource(layer, code, (int)len);
				so->last_modify_time = t;
				Graphics_BuildRenderLayer(gx->graphics, i);
			}
		}
	}
	return 0;
}

/* currently unused
static void Glslr_UpdateMousePosition(Glslr *gx)
{
	int i;
	const int max_zap_event = 16;

	if (gx->mouse.fd < 0) {
		return;
	}

	for (i = 0; i < max_zap_event; i++) {
		struct input_event ev;
		ssize_t len;
		int err;
		errno = 0;
		len = read(gx->mouse.fd, &ev, sizeof(ev));
		err = errno;
		errno = 0;
		if (len != sizeof(ev)) {
			// no more data
			break;
		}
		if (err != 0) {
			if (err == EWOULDBLOCK || err == EAGAIN) {
				// ok... try again next time            chunk.memory = malloc(1);
            chunk.size = 0;
				break;
			} else {
				printf("error on mouse-read: code %d(%s)\n", err, strerror(err));
				return;
			}
		}
		if (ev.type == EV_REL) { // relative-move event
			switch (ev.code) {
			case REL_X:
				gx->mouse.x += (int)ev.value;
				break;
			case REL_Y:
				gx->mouse.y += -(int)ev.value;
				break;
			default:
				break;
			}
		}
	}

	{
		int width, height;
		// fix mouse position
		Graphics_GetWindowSize(gx->graphics, &width, &height);
		gx->mouse.x = CLAMP(0, gx->mouse.x, width);
		gx->mouse.y = CLAMP(0, gx->mouse.y, height);
	}
}
*/

static void Glslr_SetUniforms(Glslr *gx)
{
	double t;
	double mouse_x, mouse_y;
	int width, height;

	t = GetCurrentTimeInMilliSecond() - gx->time_origin;
	Graphics_GetWindowSize(gx->graphics, &width, &height);
	mouse_x = (double)gx->mouse.x / width;
	mouse_y = (double)gx->mouse.y / height;

	Graphics_SetUniforms(gx->graphics, t / 1000.0,
	                     gx->net_input_val,
	                     mouse_x, mouse_y, drand48(), drand48());
}

static void Glslr_Render(Glslr *gx)
{
	double t = 0, vs = 0, ms = 0;
	t = GetCurrentTimeInMilliSecond();

	#ifdef VIDEO
	int framesize;
    if (gx->use_video == 1) {
        capture_video_frame(&gx->sourceparams, &framesize);
        vs = GetCurrentTimeInMilliSecond();
    }
	Graphics_Render(gx->graphics, &gx->sourceparams, &jpeg_dec);
	#else
	Graphics_Render(gx->graphics, &jpeg_dec);
	#endif

	//TODO render time works pretty weirdly, gets different results compared to nSight
	//TODO see if possible to have also sony time
	//TODO print timestamp in front of stats in second precision
	if (gx->verbose.render_time) {
        if (gx->use_video == 1) {
			ms = GetCurrentTimeInMilliSecond() - vs;
            printf("-- render time: %.1f ms (%.0f fps) / video time:  %.1f ms (%.0f fps)  \r", ms, 1000.0 / ms, vs-t, 1000.0 / (vs-t));
        } else {
			ms = GetCurrentTimeInMilliSecond() - t;
            printf("-- render time: %.1f ms (%.0f fps)\r", ms, 1000.0 / ms);
        }
	}
}

static void Glslr_AdvanceFrame(Glslr *gx)
{
	gx->frame += 1;
}

static int Glslr_Update(Glslr *gx)
{
	glfwPollEvents();
	if (Glslr_ReloadAndRebuildShadersIfNeed(gx)) {
		return 1;
	}
	if (gx->use_net) {
		dopoll(gx);
	}
	if (gx->use_sony) {
		// setup libcurl
		curl_global_init(CURL_GLOBAL_ALL);
		gx->mem.curl_handle = curl_easy_init();
		curl_easy_setopt(gx->mem.curl_handle, CURLOPT_URL, "http://192.168.122.1:60152/liveview.JPG?!1234!http-get:*:image/jpeg:*!!!!!");

		if (!gx->sony_thread_active) {
			gx->sony_thread_active = true;
			gx->mem.stop = false;
			pthread_create(&gx->thread, NULL, &getJpegData, &gx->mem);
		}
	} else {
		// stop the thread if running
		if (gx->sony_thread_active) {
			gx->mem.stop = true;
			gx->sony_thread_active = false;
		}
	}
	//Glslr_UpdateMousePosition(gx);
	Glslr_SetUniforms(gx);
	Glslr_Render(gx);
	Glslr_AdvanceFrame(gx);

	return 0;
}

static void PrintHelp(void)
{
	printf("Key:\n");
	printf("  [ or ]   offscreen scaling (defunct)\n");
	printf("  b        backbuffer ON/OFF\n");
	printf("  c        sony input ON/OFF\n");
	printf("  s        save file ON/OFF\n");
	printf("  t        FPS output ON/OFF \n");
	printf("  v        video input ON/OFF\n");
	printf("  q        exit\n");
}

void Glslr_Usage(void)
{
	printf("usage: glslr [options] <layer0.glsl> [layer1.glsl] [layer2.glsl] ...\n");
	printf("options:\n");
    printf("  window properties:\n");
    printf("    --primary-fs                            create a fullscreen window on primary monitor\n");
	printf("    --primary-res [WidthxHeight]            create a width x height window on primary monitor (default: 800x600)\n");
    printf("    --secondary-fs                          create a fullscreen window on secondary monitor\n");
	printf("    --secondary-res [WidthxHeight]          create a width x height window on secondary monitor\n");
	printf("  offscreen format:\n");
	printf("    --RGB888\n");
	printf("    --RGBA8888 (default)\n");
	printf("    --RGB565\n");
	printf("    --RGBA4444\n");
	printf("  interpolation mode:\n");
	printf("    --nearestneighbor (default)\n");
	printf("    --bilinear\n");
	printf("  wrap mode:\n");
	printf("    --wrap-clamp_to_edge\n");
	printf("    --wrap-repeat (default)\n");
	printf("    --wrap-mirror_repeat\n");
	printf("  backbuffer:\n");
	printf("    --backbuffer                            enable backbuffer (default:OFF)\n");
	printf("  network:\n");
	printf("    --net                                   enable network (default:OFF)\n");
	printf("    --tcp                                   enable TCP (default:UDP)\n");
	printf("    --port [port]                           listen on port (default:6666)\n");
	printf("    --params [n]                            number of net input params (default:0)\n");
	printf("  sony:\n");
	printf("    --sony                                  enable sony ActionCAM AS30 support\n");
    printf("  video:\n");
#ifdef VIDEO
    printf("    --vdev [device number]                  v4l2 device number (default: 0 eg. /dev/video0)\n");
#else
	printf("    No video support compiled.\n");
#endif
	printf("  saving:\n");
	printf("    --save-tga                              use TGA format instead of the default JPEG\n");
	printf("    --save-dir [dir]                        directory where to save frames\n");
	printf("    --save-file [filename]                  filename to save frames in the form: name_%%0d\n");
	printf("                                            %%0d stands for number of digits, eg. my_%%06d\n");
	printf("                                            will be saved as my_000001.jpg, my_000002.jpg, etc.. (or .tga)\n");
	printf("\n");
}

#if 0
static int Glslr_HandleKeyboadEvent(Glslr *gx)
{
	/* TODO */
	return 0;
}
#endif

static int Glslr_PrepareMainLoop(Glslr *gx)
{
	#ifdef VIDEO
	// setup video
	if (gx->video_dev_num != 666) {
		const char *fmt = "/dev/video%d";
    	snprintf(&gx->video_device[0], 12, fmt, gx->video_dev_num);
    	printf("Initializing video device: %s\n", gx->video_device);
		//TODO - try catch this
    	init_device_and_buffers(gx->video_device, &(gx->sourceparams), &(gx->capabilities));
	}
    Graphics_InitDisplayData(gx->graphics, &(gx->sourceparams));
	#else
	Graphics_InitDisplayData(gx->graphics);
	#endif

	return Graphics_AllocateOffscreen(gx->graphics);
}

// TODO implement a 'screenshot' function (just one image saved on keypress)
static void Glslr_MainLoop(Glslr *gx)
{
	for (;;) {
		switch (getchar()) {
		case 'Q':
		case 'q':
		case VEOF:      /* Ctrl+d */
		case VINTR:     /* Ctrl+c */
		case 0x7f:      /* Ctrl+c */
		case 0x03:      /* Ctrl+c */
			printf("\nexit\n");
			goto goal;
		case ']':
			// defunct
			//Glslr_ChangeScaling(gx, 1);
			break;
		case '[':
			// defunct
			//Glslr_ChangeScaling(gx, -1);
			break;
        case 'c':
        case 'C':
			Glslr_SwitchSony(gx);
			printf("Sony ACH3 input %s\n", gx->use_sony ? "ON": "OFF");
			break;
		case 's':
	    case 'S':
			Glslr_SwitchSave(gx);
			printf("Saving %s\n", gx->do_save ? "ON": "OFF");
			break;
		case 't':
		case 'T':
			gx->verbose.render_time ^= 1;
			printf("\n");
			break;
		case 'b':
			Glslr_SwitchBackbuffer(gx);
			printf("backbuffer %s\n", gx->use_backbuffer ? "ON": "OFF");
			break;
        case 'v':
			Glslr_SwitchVideo(gx);
			printf("video input %s\n", gx->use_video ? "ON": "OFF");
			break;
		case 'h':
		case '?':
			PrintHelp();
		default:
			break;
		}
		if (Glslr_Update(gx)) {
			break;
		}
	}

goal:
	return;
}

static int Glslr_AppendLayer(Glslr *gx, const char *path)
{
	SourceObject *so;
	FILE *fp;
	char code[MAX_SOURCE_BUF];
    int len, lines_before, lines_included;
    len = 0;
    lines_before = 0;
    lines_included = 0;

	GXDebug(gx, ("Glslr_AppendLayer: %s\n", path));
	fp = fopen(path, "r");
	if (fp == NULL) {
		fprintf(stderr, "file open failed: %s\n", path);
		return 1;
	}
	len = fread(code, 1, sizeof(code), fp);
	fclose(fp);
	so = SourceObject_Create(path);
    Glslr_IncludeAdditionalCode(code, &len, &lines_before, &lines_included); //handle error
	Graphics_AppendRenderLayer(gx->graphics, code, lines_before, lines_included, (int)len, (void *)so);
	return 0;
}

//TODO: Clean this up
void Glslr_IncludeAdditionalCode(char *code, int *len, int *lines_before, int *lines_included)
{
    FILE *fp;
    char inc_code[MAX_SOURCE_BUF];
    char *new_code, *new_index;
    size_t startlen, inclen, restlen, newlen;
    char *start, *index, *c, *filename;
    *lines_before = 0;
    *lines_included = 0;
    startlen = 0;
    inclen = 0;
    restlen = 0;
    index = NULL;
    start = NULL;
    c = NULL;
    filename = NULL;
    new_code = NULL;
    new_index = NULL;

    start = code;
    index = strstr(code, "//#include");
    if (index != NULL) {
        c = index;
        // move size of "//#include "
        for (int i=0; i<11; i++) {
            c++;
        }
        int i = 0;
        while ((c[i] != EOF) && (c[i] != '\n')) {
            i++;
        }

        filename = (char *)malloc((i+1) * sizeof(char));
        strncpy(filename, c, i);
        filename[i] = '\0';

		// move size of filename
        for (int j=0; j<i; j++) {
            c++;
        }
        c++;

        //read code to include
        fp = fopen(filename, "r");
        if (fp == NULL) {
            fprintf(stderr, "Include file open failed: %s\n", filename);
            exit(0);
        }
        inclen = fread(inc_code, 1, sizeof(inc_code), fp);
        fclose(fp);
        inc_code[inclen] = '\0';

        // do some math
        startlen = index - start;
        restlen = *len - startlen - 11 - i - 1; //gross
        *lines_before = Glslr_GetLineCount(code, startlen);
        *lines_included = Glslr_GetLineCount(inc_code, inclen);
        new_code = (char *)malloc((startlen + inclen + restlen + 1) * sizeof(char));
        new_index = new_code;

        // now replace the //#include
        strncpy(new_index, start, startlen);
        new_index += startlen;
        strncpy(new_index, inc_code, inclen);
        new_index += inclen;
        strncpy(new_index, c, restlen);
        newlen = startlen + inclen + restlen;
        new_code[newlen-1] = '\0';
        strcpy(code, new_code);
        *len = newlen;

		// TODO UGH OMG LOL
        fp = fopen("/tmp/kakinko", "w");
        fputs(new_code, fp);
        fclose(fp);
    }
}


int Glslr_GetLineCount(char *code, size_t size)
{
    unsigned int i = 0;
    int lines = 0;
    while (i <= size) {
        if (code[i] == '\n') {
            lines++;
        }
        i++;
    }
    return lines;
}

int Glslr_ParseArgs(Glslr *gx, int argc, const char *argv[])
{
	int i;
	int layer;
    int width, height;
    char **layers;
	char dirpath[255], filename[255];
	Graphics *g;

	g = gx->graphics;
    layers = malloc(sizeof(char*)*(argc+1));
    layer = 0;

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--debug")) {
			gx->verbose.debug = 1;
            continue;
        }
        if (!strcmp(argv[i], "--primary-fs")) {
            Graphics_SetLayout(g, Graphics_LAYOUT_PRIMARY_FULLSCREEN, 0, 0);
            continue;
        }
        if (!strcmp(argv[i], "--primary-res")) {
            if (++i>=argc) Glslr_Usage();
            if (sscanf(argv[i], "%dx%d", &width, &height) < 2) Glslr_Usage();
            Graphics_SetLayout(g, Graphics_LAYOUT_PRIMARY_RESOLUTION, width, height);
            continue;
		}
        if (!strcmp(argv[i], "--secondary-fs")) {
            Graphics_SetLayout(g, Graphics_LAYOUT_SECONDARY_FULLSCREEN, 0, 0);
            continue;
        }
        if (!strcmp(argv[i], "--secondary-res")) {
            if (++i>=argc) Glslr_Usage();
            if (sscanf(argv[i], "%dx%d", &width, &height) < 2) Glslr_Usage();
            Graphics_SetLayout(g, Graphics_LAYOUT_SECONDARY_RESOLUTION, width, height);
            continue;
        }
        if (!strcmp(argv[i], "--RGB888")) {
			Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB888);
            continue;
		}
        if (!strcmp(argv[i], "--RGBA8888")) {
			Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGBA8888);
            continue;
		}
        if (!strcmp(argv[i], "--RGB565")) {
			Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGB565);
            continue;
		}
        if (!strcmp(argv[i], "--RGBA4444")) {
			Graphics_SetOffscreenPixelFormat(g, Graphics_PIXELFORMAT_RGBA4444);
            continue;
		}
        if (!strcmp(argv[i], "--nearestneighbor")) {
			Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_NEARESTNEIGHBOR);
            continue;
		}
        if (!strcmp(argv[i], "--bilinear")) {
			Graphics_SetOffscreenInterpolationMode(g, Graphics_INTERPOLATION_MODE_BILINEAR);
            continue;
		}
        if (!strcmp(argv[i], "--wrap-clamp_to_edge")) {
			Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_CLAMP_TO_EDGE);
            continue;
		}
        if (!strcmp(argv[i], "--wrap-repeat")) {
			Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_REPEAT);
            continue;
		}
        if (!strcmp(argv[i], "--wrap-mirror_repeat")) {
			Graphics_SetOffscreenWrapMode(g, Graphics_WRAP_MODE_MIRRORED_REPEAT);
            continue;
		}
        if (!strcmp(argv[i], "--backbuffer")) {
			gx->use_backbuffer = 1;
            continue;
		}
        if (!strcmp(argv[i], "--net")) {
			gx->use_net = 1;
            continue;
		}
        if (!strcmp(argv[i], "--tcp")) {
			gx->use_tcp = 1;
            continue;
		}
        if (!strcmp(argv[i], "--port")) {
            if (++i>=argc) Glslr_Usage();
			gx->port = strtol(argv[i], NULL, 10); /* TODO handle error gnd */
            continue;
		}
        if (!strcmp(argv[i], "--params")) {
            if (++i>=argc) Glslr_Usage();
			gx->net_params=atoi(argv[i]); /* TODO handle error gnd */
            continue;
		}
		if (!strcmp(argv[i], "--sony")) {
			gx->use_sony = 1;
            continue;
		}
        if (!strcmp(argv[i], "--vdev")) {
            if (++i>=argc) Glslr_Usage();
			gx->video_dev_num=atoi(argv[i]); /* TODO handle error gnd */
            continue;
		}
		if (!strcmp(argv[i], "--save-tga")) {
            SetSaveTGA(gx);
            continue;
        }
		if (!strcmp(argv[i], "--save-dir"))
        {
            if (++i>=argc) Glslr_Usage();
            if (sscanf(argv[i], "%s", dirpath) < 1) Glslr_Usage();
            // remove trailing slash
            if (dirpath[strlen(dirpath)-1] == '/') {
                dirpath[strlen(dirpath)-1] = '\0';
            }
            // check if dir exists
            struct stat stats;
            stat(dirpath, &stats);
            if (!S_ISDIR(stats.st_mode)) {
                printf("Save directory %s doesnt exist.\nExiting.\n", dirpath);
                exit(1);
            }
            SetSaveDir(gx, dirpath);
            continue;
        }
        if (!strcmp(argv[i], "--save-file")) { // TODO exit on malformed filename
			if (++i>=argc) Glslr_Usage();
            if (sscanf(argv[i], "%s", filename) < 1) Glslr_Usage();
            SetSaveFile(gx, filename);
            continue;
        }

        // the rest is layers
        if ((argc - i) < 1) {
            printf("No layer specified\n");
            Glslr_Usage();
        }
        int length = strlen(argv[i]);
        layers[layer] = malloc((length + i) * sizeof(char));
        strcpy(layers[layer], argv[i]);
        printf("layer %d: %s\n", layer, argv[i]);
		layer++;
        layers[layer] = NULL;
	}

	// Determine the final saving path + filename
    Glslr_SetSaveDestination(gx, g);
    Graphics_SetupViewport(g); /* has to be done after args parsing but before appending layers */
    Graphics_SetBackbuffer(g, gx->use_backbuffer);
	Graphics_SetSony(g, gx->use_sony);

    // add layers
    for (i = 0; i < layer; i++) {
        printf("Opening layer %d: %s\n", i, layers[i]);
        Glslr_AppendLayer(gx, layers[i]);
    }

	// create net_input_val linked list
	netin_val *curr=NULL;
	netin_val *next=NULL;
	i = 0;
	while (i < gx->net_params) {
		curr=malloc(sizeof(netin_val));
		curr->next = next;
		curr->val = 0;
		next = curr;
		i++;
	}
	gx->net_input_val = next;
	Graphics_SetNetParams(g, gx->net_params);
	Graphics_ApplyOffscreenChange(gx->graphics);
	if (gx->use_net) {
		Glslr_Listen(gx->use_tcp, gx->port);
	}
	printf("Press h or ? for help\n");

	return (layer == 0) ? 1 : 0;
}

int Glslr_HostInitialize(void)
{
	Graphics_HostInitialize();
	return 0;
}

void Glslr_HostDeinitialize(void)
{
	Graphics_HostDeinitialize();
}

size_t Glslr_InstanceSize(void)
{
	return sizeof(Glslr);
}

int Glslr_Main(Glslr *gx)
{
	Glslr_PrepareMainLoop(gx);
	Glslr_MainLoop(gx);

	return EXIT_SUCCESS;
}
