#include "monitor.h"

BOOL CheckOneInstance()
{

    HANDLE  m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, L"GlobalMonitor\CSAPP");

    if (m_hStartEvent == NULL) {
        CloseHandle(m_hStartEvent);
        return FALSE;
    }

    // if the event already exists, we exit
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return FALSE;


    // we return true if this is the only one running
    return TRUE;
}

BOOL initSharedMemory(ControlData* cdata) {

    // OpenFileMapping
    cdata->hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
    // FILE_MAP_ALL_ACCESS - (read and write) Includes all access rights to a file-mapping object except FILE_MAP_EXECUTE 

    if (cdata->hMapFile == NULL) {
        _tprintf(TEXT("Error OpenFileMapping [%d] \n"), GetLastError());
        return FALSE;
    }

    // MapViewOfFile
    cdata->sharedTable = (SharedTable*)MapViewOfFile(cdata->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, TABLESIZE);

    if (cdata->sharedTable == NULL) {
        _tprintf(TEXT("Error: MapViewOfFile [%d] \n"), GetLastError());
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    // Open our Mutex
    cdata->hRWMutex = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME);

    if (cdata->hRWMutex == NULL) {
        _tprintf(TEXT("Error: OpenMutex [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    cdata->hMutexEmpty = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME2);

    if (cdata->hMutexEmpty == NULL) {
        _tprintf(TEXT("Error: OpenMutex [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    // Creating our semaphores
    cdata->hSemWrite = CreateSemaphore(NULL, 0, TAM_BUFFER, SEM_NAME_W);

    if (cdata->hSemWrite == NULL) {
        _tprintf(TEXT("Error: hWithoutBouncer [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        CloseHandle(cdata->hRWMutex);
        CloseHandle(cdata->hMutexEmpty);
        return FALSE;
    }


    cdata->hSemRead = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_NAME_R);

    if (cdata->hSemRead == NULL) {
        _tprintf(TEXT("Error: hWithoutBouncer [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        CloseHandle(cdata->hRWMutex);
        CloseHandle(cdata->hMutexEmpty);
        return FALSE;
    }

    /*cdata->hSemEmpty = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_NAME_E);

    if (cdata->hSemEmpty == NULL) {
        _tprintf(TEXT("Error: CreateSemaphore [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        CloseHandle(cdata->hRWMutex);
        return FALSE;
    }*/

    // Opening our events
    cdata->printTable = OpenEvent(SYNCHRONIZE, FALSE, L"PrintMonitor\CSAPP");

    if (cdata->printTable == NULL)
    {
        CloseHandle(cdata->printTable);
        return -1;
    }

    cdata->suspend = OpenEvent(SYNCHRONIZE, FALSE, L"Suspend\CSAPP");

    if (cdata->suspend == NULL)
    {
        CloseHandle(cdata->printTable);
        return -1;
    }

    return TRUE;

}

DWORD WINAPI showTable(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    SharedTable sharedTable;

    while (1) {

        WaitForSingleObject(pcd->suspend, INFINITE); // waits if the game is suspended

        /*_tprintf(TEXT("\n\n\nWidth: %d\n"), pcd->sharedTable->width);
        _tprintf(TEXT("Heigth: %d\n"), pcd->sharedTable->height);
        _tprintf(TEXT("Timer: %d\n"), pcd->sharedTable->timer);*/

        int nlinha = 1;
        int ncoluna = 1;

        _tprintf(TEXT("\n   "));

        for (int i = 0; i < pcd->sharedTable->width; i++) {

            _tprintf(TEXT("%02d "), ncoluna);
            ncoluna++;

        }

        _tprintf(TEXT("\n"));

        for (int i = 0; i < pcd->sharedTable->height; i++) {

            _tprintf(TEXT("%02d "), nlinha);

            for (int j = 0; j < pcd->sharedTable->width; j++) {

                _tprintf(TEXT("%2c "), pcd->sharedTable->board[(i * pcd->sharedTable->width) + j]);

            }
            _tprintf(TEXT("\n"));
            nlinha++;
        }

        _tprintf(TEXT("\n\n"));

        if (!pcd->sharedTable->random) { // only show the pieces when the game is in manual mode
            _tprintf(TEXT("Piece 1: ━\n"));
            _tprintf(TEXT("Piece 2: ┃\n"));
            _tprintf(TEXT("Piece 3: ┏\n"));
            _tprintf(TEXT("Piece 4: ┓\n"));
            _tprintf(TEXT("Piece 5: ┛\n"));
            _tprintf(TEXT("Piece 6: ┗\n"));
            _tprintf(TEXT("Piece 7: X\n"));
        }

        WaitForSingleObject(pcd->printTable, INFINITE);

        Sleep(2000);

    }

}

DWORD WINAPI checkLevel(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    SharedTable sharedTable;

    while (1) {

        if (pcd->sharedTable->level == -1) {
            _tprintf(TEXT("The water went out of the pipes! Type anything to quit.\n"));
            return 2;
        }
        else if (pcd->sharedTable->level == 100) {
            _tprintf(TEXT("You won the game! Type anything to quit.\n"));
            return 2;
        }
        else if (pcd->sharedTable->level == -2) {
            _tprintf(TEXT("The server closed! Type anything to quit.\n"));
            return 2;
        }

        if (WaitForSingleObject(pcd->suspend, 0) == WAIT_TIMEOUT)
            _tprintf(TEXT("The game is paused!\n"));

        Sleep(2000);

    }

}

/*DWORD WINAPI checkLife(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    SharedTable sharedTable;

    while (1) {

        WaitForSingleObject(pcd->hRWMutex, INFINITE);

        WaitForSingleObject(pcd->monitorLife, 15);

        // manda sinal de vida

        ReleaseMutex(pcd->hRWMutex);

    }

}*/

int _tmain(int argc, LPTSTR argv[]) {

    srand(time(NULL));
    ControlData cdata;
    HANDLE hThread[2];
    TCHAR command[100];

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
#endif

    // initializing shared memory
    if (!initSharedMemory(&cdata)) {
        _tprintf(TEXT("Error opening shared memory."));
        exit(1);
    }

    // we need to check if this is the first intance because of the semaphore attributes
    if (!CheckOneInstance()) {
        cdata.sharedTable->nServer = 0;
        cdata.sharedTable->nMonitor = 0;
        cdata.sharedTable->posW = 0;
        cdata.sharedTable->posR = 0;
    }

    cdata.shutdown = 0;
    // critical zone (N monitors)
    WaitForSingleObject(cdata.hRWMutex, INFINITE);
    cdata.sharedTable->nMonitor++;
    cdata.id = cdata.sharedTable->nMonitor;
    ReleaseMutex(cdata.hRWMutex);
    // Creating our threads
    hThread[0] = CreateThread(NULL, 0, showTable, &cdata, 0, NULL);

    if (hThread[0] == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        cdata.shutdown = 1;
        WaitForSingleObject(hThread[0], INFINITE);
        return -1;
    }

    hThread[1] = CreateThread(NULL, 0, checkLevel, &cdata, 0, NULL);

    if (hThread[1] == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        cdata.shutdown = 1;
        WaitForSingleObject(hThread[1], INFINITE);
        return -1;
    }

    // initializing variables used in our semaphore
    BufferCell cell;
    int count = 0, countStrTok = 2, randomPiece;
    TCHAR cm[commandsTAM], cmCopy[commandsTAM];
    TCHAR* piece = NULL;
    TCHAR* randomPieceText = NULL;
    TCHAR* nextToken = NULL;
    TCHAR* cmFirstArgument = NULL;
    TCHAR* cmSecondArgument = NULL;
    TCHAR* cmThirdArgument = NULL;
    TCHAR* cmFourthArgument = NULL;
    DWORD returnThread = NULL;
    BOOL wrong = FALSE;

    // Semaphore
    while (!cdata.shutdown) {

        cell.id = cdata.id;

        //WaitForSingleObject(cdata.hSemEmpty, INFINITE); // waits if the semaphore is empty

        /*WaitForSingleObject(cdata.hSemRead, INFINITE); // waits if the semaphore has read everything

        WaitForSingleObject(cdata.hMutexEmpty, INFINITE); // mutex for the monitor*/

        Sleep(500); // we want the "Type command:" to appear after showing the table

        if (cdata.sharedTable->random) { // choose a random piece if we are in random mode
            randomPiece = rand() % 6 + 1;

            switch (randomPiece) {

            case 1:
                randomPieceText = TEXT('━');
                break;
            case 2:
                randomPieceText = TEXT('┃');
                break;
            case 3:
                randomPieceText = TEXT('┏');
                break;
            case 4:
                randomPieceText = TEXT('┓');
                break;
            case 5:
                randomPieceText = TEXT('┛');
                break;
            case 6:
                randomPieceText = TEXT('┗');
                break;

            }

            Sleep(2100); // we want tho show the random piece after showing the table
            _tprintf(TEXT("Next piece: %c\n"), randomPieceText);

        }

        while (1) {

            if (WaitForSingleObject(hThread[1], 0) == WAIT_OBJECT_0) { // if the thread 1 ends, it's because tthe game has ended for some reason
                TerminateThread(hThread[0], 0);
                return 0;
            }

            WaitForSingleObject(cdata.suspend, INFINITE); // waits if the game is suspended

            wrong = FALSE;

            _tprintf(TEXT("Type a command: (stop s, place p s c, switch, quit)\n"));
            _getts_s(cm, commandsTAM);

            if (cdata.sharedTable->level != -1 && cdata.sharedTable->level != 100) {

                _tcscpy_s(cmCopy, commandsTAM, cm);

                piece = wcstok_s(cm, TEXT(" "), &nextToken);
                cmFirstArgument = piece;

                while (piece != NULL) { // separate the command

                    if (countStrTok == 2) {
                        piece = wcstok_s(NULL, TEXT(" "), &nextToken);
                        cmSecondArgument = piece;
                    }
                    else if (countStrTok == 3) {
                        piece = wcstok_s(NULL, TEXT(" "), &nextToken);
                        cmThirdArgument = piece;
                    }
                    else if (countStrTok == 4) {
                        piece = wcstok_s(NULL, TEXT(" "), &nextToken);
                        cmFourthArgument = piece;
                    }

                    if (countStrTok == 4)
                        break;

                    countStrTok++;

                }

                if (_tcscmp(cmFirstArgument, TEXT("stop")) == 0) { // if the user typed stop (...)

                    if (cmSecondArgument == NULL) {
                        _tprintf(TEXT("You need 2 parameters for this command!\n"));
                        wrong = TRUE;
                        break;
                    }

                    if (cmThirdArgument != NULL || cmFourthArgument != NULL) {
                        _tprintf(TEXT("You need 2 parameters for this command!\n"));
                        wrong = TRUE;
                        break;
                    }

                    if (_wtoi(cmSecondArgument) == 0) {
                        _tprintf(TEXT("You have to introduce an integer in the second parameter! (seconds)\n"));
                        wrong = TRUE;
                        break;
                    }

                    if (_wtoi(cmSecondArgument) < 1 || _wtoi(cmSecondArgument) > 100) {
                        _tprintf(TEXT("Time unvalid! (1 - 100)\n"));
                        wrong = TRUE;
                        break;
                    }

                    if (!wrong) {
                        _tcscpy_s(cell.value, TAM, cmCopy);
                        _tprintf(TEXT("Water flow stoped!\n"));
                        break;
                    }

                    break; // we go to the copymemory part

                }
                else if (_tcscmp(cmFirstArgument, TEXT("place")) == 0) { // if the user place stop (...)

                    if (!cdata.sharedTable->random) { // if the game is in manual mode

                        if (cmSecondArgument == NULL || cmThirdArgument == NULL || cmFourthArgument == NULL) {
                            _tprintf(TEXT("You need 4 parameters for this command!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmSecondArgument) == 0 || _wtoi(cmThirdArgument) == 0 || _wtoi(cmFourthArgument) == 0) {
                            _tprintf(TEXT("You have to introduce an integer in the second, third and fourth parameter! (piece, line and collum)\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmSecondArgument) < 1 || _wtoi(cmSecondArgument) > 7) {
                            _tprintf(TEXT("Piece unvalid! Choose a piece between 1 and 7.\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmThirdArgument) < 0 || _wtoi(cmThirdArgument) > cdata.sharedTable->height) {
                            _tprintf(TEXT("You have to introduce a valid line!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmFourthArgument) < 0 || _wtoi(cmFourthArgument) > cdata.sharedTable->width) {
                            _tprintf(TEXT("You have to introduce a valid collum!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmFourthArgument) * _wtoi(cmThirdArgument) == cdata.sharedTable->posInit) {
                            _tprintf(TEXT("You can't place anything in the inicial flow position!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmFourthArgument) * _wtoi(cmThirdArgument) == cdata.sharedTable->posEnd) {
                            _tprintf(TEXT("You can't place anything in the water receiver!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (cdata.sharedTable->board[((_wtoi(cmThirdArgument) - 1) * cdata.sharedTable->width) + (_wtoi(cmFourthArgument) - 1)] == TEXT('X')) {
                            _tprintf(TEXT("You can't place anything on top of a block (X)!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (!wrong) {
                            _tcscpy_s(cell.value, TAM, cmCopy);
                            _tprintf(TEXT("Command 'place' sent!\n"));
                        }

                        break; // we go to the copymemory part

                    }
                    else { // if the game is using the random mode to place pieces

                        // we just need to know where the user wants to place the piece

                        BOOL wrong = FALSE;

                        if (cmSecondArgument == NULL || cmThirdArgument == NULL) {
                            _tprintf(TEXT("You need 3 parameters for this command!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmSecondArgument) == 0 || _wtoi(cmThirdArgument) == 0) {
                            _tprintf(TEXT("You have to introduce an integer in the second and third parameter! (line and collum)\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmSecondArgument) < 0 || _wtoi(cmSecondArgument) > cdata.sharedTable->height) {
                            _tprintf(TEXT("You have to introduce a valid line!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (_wtoi(cmThirdArgument) < 0 || _wtoi(cmThirdArgument) > cdata.sharedTable->width) {
                            _tprintf(TEXT("You have to introduce a valid collum!\n"));
                            wrong = TRUE;
                            break;
                        }

                        if (!wrong) {
                            /*
                            //_tprintf(TEXT("debug 1: %s\n"), cmFirstArgument);
                            //_tprintf(TEXT("debug 2: %s\n"), cmSecondArgument);
                            //_tprintf(TEXT("debug 3: %s\n"), cmThirdArgument);
                            //_tprintf(TEXT("debug 4: %s\n"), cmFourthArgument);

                            cmFourthArgument = cmThirdArgument;
                            //cmFourthArgument = TEXT("fds");
                            //_tcscpy_s(cmFourthArgument, TAM, cmThirdArgument);
                            cmThirdArgument = cmSecondArgument;
                            //cmThirdArgument = TEXT("gfdgdgfd");
                            //_tcscpy_s(cmThirdArgument, TAM, cmSecondArgument);

                            _itot_s(randomPiece, cmSecondArgument, commandsTAM, 2);

                            //_tprintf(TEXT("debug 1: %s\n"), cmFirstArgument);
                            //_tprintf(TEXT("debug 2: %s\n"), cmSecondArgument);
                            //_tprintf(TEXT("debug 3: %s\n"), cmThirdArgument);
                            //_tprintf(TEXT("debug 4: %s\n"), cmFourthArgument);

                            TCHAR cmRandom[100];
                            //TCHAR teste[1000];
                            //_tcscpy_s(teste, 1000, TEXT("testeeeee"));
                            _tprintf(TEXT("tou\n"));
                            //strcat_s(cmRandom, sizeof(cmRandom) + 100, cmFirstArgument);
                            //StringCchCatA(cmRandom, sizeof(cmRandom), cmFirstArgument);
                            _tcscat_s(cmRandom, sizeof(cmFirstArgument) + 300, cmFirstArgument);
                            _tprintf(TEXT("tou\n"));
                            //strcat_s(cmRandom, sizeof(cmRandom) + 100, cmSecondArgument);
                            //StringCchCatA(cmRandom, sizeof(cmRandom), cmSecondArgument);
                            _tcscat_s(cmRandom, sizeof(cmSecondArgument), cmSecondArgument);
                            _tprintf(TEXT("tou\n"));
                            //strcat_s(cmRandom, sizeof(cmRandom) + 100, cmThirdArgument);
                            //StringCchCatA(cmRandom, sizeof(cmRandom), cmThirdArgument);
                            _tcscat_s(cmRandom, sizeof(cmThirdArgument), cmThirdArgument);
                            _tprintf(TEXT("tou penultimo\n"));
                            //strcat_s(cmRandom, sizeof(cmRandom) + 100, cmFourthArgument);
                            //StringCchCatA(cmRandom, sizeof(cmRandom), cmFourthArgument);
                            _tcscat_s(cmRandom, sizeof(cmFourthArgument), cmFourthArgument);
                            _tprintf(TEXT("tou ultimo\n"));

                            for (int i = 0; i < _tcslen(cmRandom); i++)
                                _tprintf(TEXT("%c"), cmRandom[i]);

                            _tprintf(TEXT("\n\n"));

                            _tprintf(TEXT("Comando RANDOM DEBUG: %s\n"), cmRandom);

                            _tcscpy_s(cell.value, TAM, cmRandom);*/

                            _tcscpy_s(cell.value, TAM, TEXT("place 3 4 5")); // this is only while this command isn't working (we are always placing the same piece in the same spot)
                            _tprintf(TEXT("Command 'place' sent!\n"));

                        }

                        break; // we go to the copymemory part

                    }

                }
                else if (_tcscmp(cmFirstArgument, TEXT("switch")) == 0) { // if the user typed switch (...)

                    if (cmSecondArgument != NULL || cmThirdArgument != NULL || cmFourthArgument != NULL) {
                        _tprintf(TEXT("You need only 1 parameters for this command!\n"));
                        wrong = TRUE;
                        break;
                    }

                    if (!wrong) {
                        _tcscpy_s(cell.value, TAM, cm);

                        if (!cdata.sharedTable->random)
                            _tprintf(TEXT("Game changed to random mode!\n"));
                        else
                            _tprintf(TEXT("Game changed to manual mode!\n"));
                    }

                    break; // we go to the copymemory part

                }
                else if (_tcscmp(cmFirstArgument, TEXT("quit")) == 0) {
                    TerminateThread(hThread[0], 0);
                    TerminateThread(hThread[1], 0);
                    return 0;
                }
                else
                    _tprintf(TEXT("Command unknown!\n")); // we keep inside the while(1)

            }

        }

        WaitForSingleObject(cdata.hSemRead, INFINITE); // waits if the semaphore has read everything

        WaitForSingleObject(cdata.hMutexEmpty, INFINITE); // mutex for the monitor

        if (!wrong) { // if the command is typed correct
            CopyMemory(&cdata.sharedTable->command[cdata.sharedTable->posW], &cell, sizeof(BufferCell));
        }
        else { // if the command is typed wrong the server receives that information
            _tcscpy_s(cell.value, TAM, TEXT("wrong"));
            CopyMemory(&cdata.sharedTable->command[cdata.sharedTable->posW], &cell, sizeof(BufferCell));
        }

        cdata.sharedTable->posW++; // we increment the position in our circular buffer

        if (cdata.sharedTable->posW == TAM_BUFFER) { // if we already have read the 5 cells
            cdata.sharedTable->posW = 0; // we go to the first one again
        }

        ReleaseMutex(cdata.hMutexEmpty); // release the mutex for the monitor

        //ReleaseSemaphore(cdata.hSemRead, 1, NULL); // decremets the counter of the semaphore for the reading

        ReleaseSemaphore(cdata.hSemWrite, 1, NULL); // decremets the counter of the semaphore for the writing

        count++;

        //_tprintf(_T("%d produziu %s \n"), cdata.id, cell.value);

        // initialize once again the variables since they were changed through the cycle
        cmFirstArgument = NULL;
        cmSecondArgument = NULL;
        cmThirdArgument = NULL;
        cmFourthArgument = NULL;
        countStrTok = 2;
        wrong = FALSE;

        Sleep(1000);

    }

    // end of monitor.c -----------------------------------------------------------------------------------------------------------------------------------------------

    WaitForSingleObject(hThread[0], INFINITE);
    WaitForSingleObject(hThread[1], INFINITE);
    UnmapViewOfFile(cdata.sharedTable);
    CloseHandle(hThread[0]);
    CloseHandle(hThread[1]);
    CloseHandle(cdata.hMapFile);
    CloseHandle(cdata.hRWMutex);
    CloseHandle(cdata.hMutexEmpty);
    CloseHandle(cdata.hSemWrite);
    CloseHandle(cdata.hSemRead);
    //CloseHandle(cdata.hPipe);

}