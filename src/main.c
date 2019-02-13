#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "config.h"
#include "base.h"
#include "glslr.h"

#include <fcntl.h>
#include <termios.h>

static void PrintHead(void)
{
	printf("glslr version %x.%x\n", VERSION_MAJOR, VERSION_MINOR);
	printf("build %s\n", __DATE__);
}


int main(int argc, char *argv[])
{
	int ret;
	fcntl(0, F_SETFL, O_NONBLOCK);

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

	return ret;
}
