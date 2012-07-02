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

// implementation of broodwars retarded calling of functions
void EDI_Call(DWORD EDI, DWORD functionToCall) {
    static char callingCode[] = {
        0x57,                               // 57               PUSH EDI
        0xBF, 0x00, 0x00, 0x00, 0x00,       // BF   78563412    MOV EDI, 12345678
        0xFF, 0x15, 0x00, 0x00, 0x00, 0x00, // FF15 21436587    CALL DWORD PTR DS:[87654321]
        0x5F,                               // 5F               POP EDI
        0xC3                                // C3               RETN
    };

    static DWORD func = functionToCall;
    *(DWORD*)(callingCode+3) = EDI;
    *(DWORD*)(callingCode+9) = (DWORD)&func;

    void (__cdecl *CallingHelper)() = (void(__cdecl*)()) callingCode;
    CallingHelper();
}


void EAX_PUSH_Call(DWORD EAX, DWORD PUSH, DWORD functionToCall) {
    static char callingCode[] = {
        0x50,                               // 57               PUSH EAX
        0x68, 0x00, 0x00, 0x00, 0x00,       // 68 78563412      PUSH 12345678
        0xB8, 0x00, 0x00, 0x00, 0x00,       // B8 78563412      MOV EAX, 12345678
        0xFF, 0x15, 0x00, 0x00, 0x00, 0x00, // FF15 21436587    CALL DWORD PTR DS:[87654321]
        0x58,                               // 5F               POP EAX
        0xC3                                // C3               RETN
    };

    static DWORD func = functionToCall;
    *(DWORD*)(callingCode+2) = PUSH;
    *(DWORD*)(callingCode+7) = EAX;
    *(DWORD*)(callingCode+13) = (DWORD)&func;

    void (__cdecl *CallingHelper)() = (void(__cdecl*)()) callingCode;
    CallingHelper();
}

