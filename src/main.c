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
		Glslr_Usage();
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
