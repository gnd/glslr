/* -*- Mode: c; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil; -*- */

#ifndef INCLUDED_PJ_H
#define INCLUDED_PJ_H

typedef struct PJContext_ PJContext;
void udpmakeoutput(char *buf, PJContext *pj);

int PJContext_HostInitialize(void);
void PJContext_HostDeinitialize(void);

size_t PJContext_InstanceSize(void);
int PJContext_Construct(PJContext *pj);
void PJContext_Destruct(PJContext *pj);
int PJContext_ParseArgs(PJContext *pj, int argc, const char *argv[]);
int PJContext_Main(PJContext *pj);


#endif
