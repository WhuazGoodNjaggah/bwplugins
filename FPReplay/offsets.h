DWORD* pGameTicks   = (DWORD*)0x57F23C;
DWORD* pScreenX     = (DWORD*)0x628448;
DWORD* pScreenY     = (DWORD*)0x628470;
WORD*  pF2ScreenX   = (WORD*) 0x57F270;
WORD*  pF2ScreenY   = (WORD*) 0x57F272;
DWORD* pCurPlayer   = (DWORD*)0x512688;

DWORD* pIsIngame    = (DWORD*)0x6556E0;
DWORD* pIsReplay    = (DWORD*)0x6D0F14;
char*  pReplayPath  = (char*) 0x57FD3C;
char*  pBWFolder    = (char*) 0x1505E4C8;

void (__cdecl *BWFXN_DisplayText)(const char* message, DWORD player) = (void(__cdecl*)(const char*, DWORD)) 0x0048D1C0;


HWND hWndBW = 0;
void BW_SetScreenPos_SM(DWORD xPos, DWORD yPos) {
    if (hWndBW == 0) {
        hWndBW = FindWindow("SWarClass", 0);
    }
    *(pF2ScreenX + *pCurPlayer *2) = (WORD)xPos;
    *(pF2ScreenY + *pCurPlayer *2) = (WORD)yPos;

    // send F2 to the BW window
    SendMessage(hWndBW, WM_COMMAND, 0x19C7F, 0);
}
