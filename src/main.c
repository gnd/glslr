#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "base.h"
#include "glslr.h"

#ifdef USE_TERMIOS
#include <fcntl.h>
#include <termios.h>
#endif


static void PrintHead(void)
{
	printf("glslr version %x.%x\r\n", VERSION_MAJOR, VERSION_MINOR);
	printf("build %s\r\n", __DATE__);
}

static void PrintCommandUsage(void)
{
	printf("usage: glslr [options] <layer0.glsl> [layer1.glsl] [layer2.glsl] ...\r\n");
	printf("options:\r\n");
	printf("  offscreen format:\r\n");
	printf("    --RGB888\r\n");
	printf("    --RGBA8888 (default)\r\n");
	printf("    --RGB565\r\n");
	printf("    --RGBA4444\r\n");
	printf("  interpolation mode:\r\n");
	printf("    --nearestneighbor (default)\r\n");
	printf("    --bilinear\r\n");
	printf("  wrap mode:\r\n");
	printf("    --wrap-clamp_to_edge\r\n");
	printf("    --wrap-repeat (default)\r\n");
	printf("    --wrap-mirror_repeat\r\n");
	printf("  backbuffer:\r\n");
	printf("    --backbuffer   enable backbuffer (default:OFF)\r\n");
	printf("  network:\r\n");
	printf("    --net	enable network (default:OFF)\r\n");
	printf("    --tcp	enable TCP (default:UDP)\r\n");
	printf("    --port	listen on port (default:6666)\r\n");
	printf("    --params	number of net input params (default:0)\r\n");
	printf("\r\n");
}

int main(int argc, char *argv[])
{
	int ret;

#ifdef USE_TERMIOS
	struct termios ios_old, ios_new;

	tcgetattr(STDIN_FILENO, &ios_old);
	tcgetattr(STDIN_FILENO, &ios_new);
	cfmakeraw(&ios_new);
	tcsetattr(STDIN_FILENO, 0, &ios_new);
	fcntl(0, F_SETFL, O_NONBLOCK);
#endif

	Glslr_HostInitialize();

	PrintHead();
	ret = EXIT_SUCCESS;

	if (argc == 1) {
		PrintCommandUsage();
	} else {
		Glslr *gx;
		gx = malloc(Glslr_InstanceSize());
		Glslr_Construct(gx);
		if (Glslr_ParseArgs(gx, argc, (const char **)argv) == 0) {
			ret = Glslr_Main(gx);
		}
		Glslr_Destruct(gx);
		free(gx);
	}

	Glslr_HostDeinitialize();

#ifdef USE_TERMIOS
	tcsetattr(STDIN_FILENO, 0, &ios_old);
#endif
	return ret;
}
