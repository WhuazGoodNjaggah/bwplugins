#ifndef REPLAY_FILE_H
#define REPLAY_FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

typedef struct fpr_data {
    DWORD   timeTicks;
    WORD    xPos;
    WORD    yPos;
} FPR_DATA;

typedef struct replay_footer {
    int   version;
    DWORD magic1;
    DWORD magic2;
    DWORD magic3;
    int   sizeOfData;
} REPLAY_FOOTER;


FPR_DATA* GetFPRData(const char* replayPath, int* nElements);

void WriteFPRData(const char* replayPath, FPR_DATA* data, int nElements);

#endif //REPLAY_FILE_H
