#include "replay_file.h"
#include "ezlib/easyzlib.h"

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
    FILE* fpReplay = fopen(replayPath, "rb");
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

    // read the compressed data
    unsigned char* pCompressedData = (unsigned char*)malloc(footer.sizeOfData);
    fpos_t posData = posFooter - footer.sizeOfData;
    fsetpos(fpReplay, &posData);
    fread(pCompressedData, footer.sizeOfData, 1, fpReplay);
    fclose(fpReplay);

    // uncompress the data
    long uncompressedSize = 1;
    unsigned char* pUncompressedData = (unsigned char*)malloc(uncompressedSize);
    int nErr = ezuncompress( pUncompressedData, &uncompressedSize, pCompressedData, footer.sizeOfData );
    if ( nErr == EZ_BUF_ERROR ) {
        free(pUncompressedData);
        pUncompressedData = (unsigned char*)malloc(uncompressedSize); // enough room now
        nErr = ezuncompress( pUncompressedData, &uncompressedSize, pCompressedData, footer.sizeOfData );
    }
    switch (nErr) {
        case EZ_NO_ERROR:
        break;
        case EZ_STREAM_ERROR:   //	-2	pDest is NULL
            MessageBox(0,"easyzlib: pDest is NULL.","ERROR.",0);
            break;
        case EZ_DATA_ERROR:	    // -3	corrupted pSrc passed to ezuncompress
            MessageBox(0,"easyzlib: corrupted pSrc passed to ezuncompress.","ERROR.",0);
            break;
        case EZ_MEM_ERROR:	    // -4	out of memory
            MessageBox(0,"easyzlib: out of memory.","ERROR.",0);
            break;
        case EZ_BUF_ERROR:	    // -5	pDest length is not enough
            MessageBox(0,"easyzlib: pDest length is not enough.","ERROR.",0);
            break;
        default:
            MessageBox(0,"Wrong Error code for easyzlib.","ERROR.",0);
            break;
    }

    FPR_DATA* pData = (FPR_DATA*)pUncompressedData;
    *nElements = uncompressedSize / sizeof(FPR_DATA);
    return pData;
}

void WriteFPRData(const char* replayPath, FPR_DATA* data, int nElements) {
    REPLAY_FOOTER footer;
    footer.version = 1;
    footer.magic1 = MAGIC_1;
    footer.magic2 = MAGIC_2;
    footer.magic3 = MAGIC_3;

    // compress the data
    long compressedSize = EZ_COMPRESSMAXDESTLENGTH(nElements * sizeof(FPR_DATA));
    unsigned char* pCompressedData = (unsigned char*)malloc(compressedSize);
    ezcompress(pCompressedData, &compressedSize, (unsigned char*)data, nElements * sizeof(FPR_DATA));

    // write the data to the file
    footer.sizeOfData = compressedSize;
    FILE* fpReplay = fopen(replayPath, "ab");
    fwrite(pCompressedData, footer.sizeOfData, 1, fpReplay);
    fwrite(&footer, sizeof(REPLAY_FOOTER), 1, fpReplay);
    fclose(fpReplay);

    free(pCompressedData);
}
