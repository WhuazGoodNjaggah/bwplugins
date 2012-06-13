#include "replay_file.h"

#define MAGIC_1 0xBADC0DED
#define MAGIC_2 0xDEADBEEF
#define MAGIC_3 0x12345678

int FileSize(FILE* pFile) {
    int lSize = 0;
    fseek (pFile , 0 , SEEK_END);
    lSize = ftell (pFile);
    rewind (pFile);
    return lSize;
}

FPR_DATA* GetFPRData(const char* replayPath, int* nElements) {
    REPLAY_FOOTER footer;
    FILE* fpReplay = fopen(replayPath, "r");
    fpos_t posBegin;
    fgetpos(fpReplay, &posBegin);
    int fileSize = FileSize(fpReplay);
    fpos_t posFooter = posBegin + fileSize - sizeof(REPLAY_FOOTER);
    fsetpos(fpReplay, &posFooter);
    fread(&footer, sizeof(REPLAY_FOOTER), 1, fpReplay);

    // check if the footer is valid
    if (footer.magic1 != MAGIC_1 || footer.magic2 != MAGIC_2 || footer.magic3 != MAGIC_3) {
        *nElements = 0;
        return 0;
    }

    // read all the data
    FPR_DATA* pData = (FPR_DATA*)malloc(footer.sizeOfData);
    fpos_t posData = posFooter - footer.sizeOfData;
    fsetpos(fpReplay, &posData);
    fread(pData, footer.sizeOfData, 1, fpReplay);
    fclose(fpReplay);

    *nElements = footer.sizeOfData / sizeof(FPR_DATA);
    return pData;
}

void WriteFPRData(const char* replayPath, FPR_DATA* data, int nElements) {
    REPLAY_FOOTER footer;
    footer.version = 1;
    footer.magic1 = MAGIC_1;
    footer.magic2 = MAGIC_2;
    footer.magic3 = MAGIC_3;
    footer.sizeOfData = nElements * sizeof(FPR_DATA);
    FILE* fpReplay = fopen(replayPath, "ab");
    fwrite(data, footer.sizeOfData, 1, fpReplay);
    fwrite(&footer, sizeof(REPLAY_FOOTER), 1, fpReplay);
    fclose(fpReplay);
}
