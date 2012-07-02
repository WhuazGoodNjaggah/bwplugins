// FPReplay BWL4 Plugin

#define BWLAPI 4
#define STARCRAFTBUILD 13   // 1.16.1
#define WINVER  0x0501      // minimum winxp

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>

#include "replay_file.h"
#include "offsets.h"


// Global vars
char    dll_path[MAX_PATH];
char    dllFolder[MAX_PATH];
char    settingsPath[MAX_PATH];
char    logFilePath[MAX_PATH];
FILE*   fpLogFile;
DWORD   initTickCount = 0;
bool    enableFPPlay = true;
WNDPROC bwWndProc_Old = 0;

DWORD   oldProtect = 0;

HANDLE  hThread = 0;
HANDLE  hBroodWar = 0;
DWORD   threadId = 0;

// prototpes
DWORD WINAPI mainThread( LPVOID lpParam );
LRESULT CALLBACK BWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void RecordScreenPositions();
void PlaybackScreenPositions();


void _log(const char* msg) {
    fprintf(fpLogFile, "%i:\t%s\r\n", (int)(GetTickCount() - initTickCount) / 100, msg);
    fflush(fpLogFile);
}

void OpenLogFile() {
    // check what prog we are attached to
    if (GetModuleHandle("storm.dll") == NULL) {
        sprintf(logFilePath, "%s%s", dllFolder, "FPReplay.cl.log");
    } else {
        sprintf(logFilePath, "%s%s", dllFolder, "FPReplay.bw.log");
    }
    fpLogFile = fopen(logFilePath, "a+");

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    fprintf(fpLogFile, "%s\r\n\t%s", "------- FPReplay attached. -------", asctime(timeinfo));
    initTickCount = GetTickCount();
}

struct ExchangeData {
	int iPluginAPI;
	int iStarCraftBuild;
	BOOL bNotSCBWmodule;                //Inform user that closing BWL will shut down your plugin
	BOOL bConfigDialog;                 //Is Configurable
};

extern "C" BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {

	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
            if (GetModuleFileName(hModule, dll_path, 255) == 0) {
                MessageBox(0,"Failed to get the path to the module.\nExiting.", "ERROR in ICCup Plugin Loader", 0);
                return FALSE;
            }
            if (strrchr(dll_path, '\\') == 0) {
                MessageBox(0,"Failed to get plugin folder.\nExiting.", "ERROR in ICCup Plugin Loader", 0);
                return FALSE;
            }
            strncpy(dllFolder, dll_path, strrchr(dll_path, '\\') - dll_path + 1);

            OpenLogFile();

            // read the settings
            _log("Read the settings.");
            sprintf(settingsPath, "%s%s", dllFolder, "fpreplay.ini");
            // attached to CL or SCBW?
            if (GetModuleHandle("storm.dll") == NULL) return TRUE;

            _log("Create the main thread.");
            // Create a thread to check for the game state
            hThread = CreateThread( NULL, 0, mainThread, 0, 0, NULL);

			return TRUE;
		case DLL_THREAD_ATTACH:
            break;
		case DLL_THREAD_DETACH:
            break;
		case DLL_PROCESS_DETACH:
            // close the log file
            _log("Close log file.");
            fprintf(fpLogFile, "%s", "\r\n\r\n");
            fclose(fpLogFile);
			break;
	}

	return TRUE;
}

// GetPluginAPI and GetData get called during the startup of the Launcher and gather information about the plugin
extern "C" __declspec(dllexport) void GetPluginAPI(ExchangeData &Data) {
	// BWL Gets version from Resource - VersionInfo (which is braindamaged)
	Data.iPluginAPI      = BWLAPI;
	Data.iStarCraftBuild = STARCRAFTBUILD;
	Data.bConfigDialog   = true;
	Data.bNotSCBWmodule  = false;
}

extern "C" __declspec(dllexport) void GetData(char *name, char *description, char *updateurl) {
	// if necessary you can add Initialize function here
	// possibly check CurrentCulture (CultureInfo) to localize your DLL due to system settings

	strcpy(name,        "FirstPerson Replay");
	strcpy(description, "Author WhuazGoodNjaggah\r\n\r\nThis plugin lets you record and watch replays in first person view.");
	strcpy(updateurl,   "http://213.133.99.163/bw/plugins/fpreplay/");
}

// Called when the user clicks on Config
extern "C" __declspec(dllexport) BOOL OpenConfig() {
	// If you set "Data.bConfigDialog = true;" at function GetPluginAPI then
	// BWLauncher will call this function if user clicks Config button
	char cmdLine[300];
	sprintf(cmdLine, "start notepad.exe \"%s\"", settingsPath);
	system(cmdLine);
	return true; //everything OK
}

// ApplyPatchSuspended and ApplyPatch get
// called during the startup of Starcraft in the Launcher process
// the hProcess passed to them is shared between all plugins, so don't close it.
// Best practice is duplicating(DuplicateHandle from Win32 API) it if you want to use if after these function returns
extern "C" __declspec(dllexport) bool ApplyPatchSuspended(HANDLE hProcess, DWORD dwProcessID) {
	// This function is called in the Launcher process while Starcraft is still suspended
	// Durning the suspended process some modules of starcraft.exe may not yet exist.

	return true; //everything OK
}

extern "C" __declspec(dllexport) bool ApplyPatch(HANDLE hProcess, DWORD dwProcessID) {
	// This function is called in the Launcher process after the Starcraft window has been created
    _log("ApplyPatch()");

    void* pDllPath = VirtualAllocEx(hProcess, 0, MAX_PATH, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (pDllPath == NULL) return FALSE;

    DWORD ret = WriteProcessMemory( hProcess, pDllPath, (void*)dll_path, strlen(dll_path) + 1, NULL );
    if (ret == 0) return FALSE;

    LPTHREAD_START_ROUTINE loadLibAddr = (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("Kernel32"), "LoadLibraryA");
    HANDLE hThread = CreateRemoteThread( hProcess, NULL, 0, loadLibAddr, pDllPath, 0, NULL );

    WaitForSingleObject( hThread, INFINITE );

	return true; //everything OK
}


DWORD WINAPI mainThread( LPVOID lpParam ) {
    Sleep(5000); // sleep for 5 seconds so the BroodWar window may be loaded

    // Subclass the BroodWar window proc
    HWND hWndBW = FindWindow("SWarClass", 0);
    bwWndProc_Old = (WNDPROC)SetWindowLong(hWndBW, GWL_WNDPROC, (LONG)BWWndProc);

    // create the output directory
    _log("create the output directory");
    char fprepsDir[MAX_PATH];
    sprintf(fprepsDir, "%smaps\\replays\\fpreps\\", pBWFolder);
    CreateDirectory(fprepsDir, NULL);

    _log("enter main loop");
    // main loop, which checks for fp-replays
    while(FindWindow("SwarClass", 0)) {
        bool isIngame = *pIsIngame == 1;
        bool isReplay = *pIsReplay == 1;

        if (isIngame && !isReplay) {
            // record the screen positions
            RecordScreenPositions();
            continue;
        }
        if (isIngame && isReplay) {
            // playback of the fpr data
            PlaybackScreenPositions();
            continue;
        }

        Sleep(500);
    }
    return 1;
}

void RecordScreenPositions() {
    _log("start screen position recording");
    char lastFPReplayPath[MAX_PATH];
    char newFPRepPath[MAX_PATH];
    sprintf(lastFPReplayPath, "%smaps\\replays\\lastfpreplay.rep", pBWFolder);

    FPR_DATA* pFprData = (FPR_DATA*)malloc(2000000); // lets hope 2 megabytes is enough :D
    int counter = 0;
    int lastScreenX  = 0;
    int lastScreenY  = 0;

    // sleep a little, maybe less crashes then (needs better fix)
    Sleep(500);
    BWFXN_DisplayText("Start recording screen positions.", 0);
    while (*pIsIngame == 1) {
        int gameTick = *pGameTicks;
        int screenX  = *pScreenX;
        int screenY  = *pScreenY;

        // has something changed?
        if (screenX != lastScreenX || screenY != lastScreenY) {
            FPR_DATA fpr_data;
            fpr_data.timeTicks = gameTick;
            fpr_data.xPos = screenX;
            fpr_data.yPos = screenY;
            memcpy(&pFprData[counter], &fpr_data, sizeof(FPR_DATA));

            lastScreenX = screenX;
            lastScreenY = screenY;
            counter++;
        }

        Sleep(15);
    }

    _log("save the replay");
    char name[] = "lastfpreplay";
    EAX_PUSH_Call((DWORD)name, 1, 0x4DFAB0);

    _log("copy the replay");
    // try to copy the last replay until it works (gnihihihihi)
    int i = 0;
    do {
        sprintf(newFPRepPath, "%smaps\\replays\\fpreps\\%04i.rep", pBWFolder, i);
        i++;
    } while (CopyFile(lastFPReplayPath, newFPRepPath, TRUE) == 0);

    _log("write data to copied replay");
    WriteFPRData(newFPRepPath, pFprData, counter);

    _log("remove the collected data from ram and delete last fp replay.");
    free(pFprData);
    DeleteFile(lastFPReplayPath);
}

void PlaybackScreenPositions() {
    int counter = 0;
    int nElements = 0;

    _log("Load replay data.");
    FPR_DATA* pFprData = GetFPRData(pReplayPath, &nElements);
    if (nElements > 0) {
        BWFXN_DisplayText("First Person Replay detected.", 0);
        if (enableFPPlay) {
            BWFXN_DisplayText("FPPlay enabled.", 0);
        } else {
            BWFXN_DisplayText("FPPlay disabled.", 0);
        }
    }

    _log("play the loaded data");
    while (*pIsIngame == 1 && *pIsReplay == 1) {
        if (counter < nElements) {
            FPR_DATA curData = *(pFprData + counter);
            if (*pGameTicks >= curData.timeTicks) {
                if (enableFPPlay) {
                    BW_SetScreenPos_SM(curData.xPos, curData.yPos);
                }
                counter++;
            }
            Sleep(15);
        } else {
            // no data, so wait until the replay is done
            Sleep(1000);
        }
    }

    // free any data that has been allocated
    if (pFprData != 0) {
        _log("free the data.");
        // free the data
        free(pFprData);
    }
}

LRESULT CALLBACK BWWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_KEYUP && GetAsyncKeyState(VK_CONTROL)) {
        // check if F1 to F8 has been pressed
        if (wParam == 0x59) {
            enableFPPlay = !enableFPPlay;
            if (enableFPPlay) {
                BWFXN_DisplayText("FPPlay enabled.", 0);
            } else {
                BWFXN_DisplayText("FPPlay disabled.", 0);
            }
        }
    }
    return CallWindowProc(bwWndProc_Old, hwnd, uMsg, wParam, lParam);
}
