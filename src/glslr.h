#ifndef INCLUDED_PJ_H
#define INCLUDED_PJ_H

typedef struct Glslr_ Glslr;
void udpmakeoutput(char *buf, Glslr *gx);

int Glslr_HostInitialize(void);
void Glslr_HostDeinitialize(void);

size_t Glslr_InstanceSize(void);
int Glslr_Construct(Glslr *gx);
void Glslr_Destruct(Glslr *gx);
int Glslr_ParseArgs(Glslr *gx, int argc, const char *argv[]);
int Glslr_Main(Glslr *gx);
void Glslr_Usage(void);
int Glslr_GetLineCount(char *code, size_t size);
void Glslr_IncludeAdditionalCode(char *code, int *len, int *lines_before, int *lines_included);


#endif