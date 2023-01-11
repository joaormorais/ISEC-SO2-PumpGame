#include "server.h"

BOOL CheckOneInstance()
{

    HANDLE  m_hStartEvent = CreateEventW(NULL, FALSE, FALSE, L"GlobalServer\CSAPP");

    if (m_hStartEvent == NULL)
    {
        CloseHandle(m_hStartEvent);
        return FALSE;
    }

    // if the event already exists, we exit
    if (GetLastError() == ERROR_ALREADY_EXISTS) {

        CloseHandle(m_hStartEvent);
        m_hStartEvent = NULL;
        _tprintf(TEXT("Error! Server is already runing."));
        return FALSE;
    }

    // we return true if this is the only one running
    return TRUE;
}

BOOL initSharedMemory(ControlData* cdata) {

    // CreateFileMapping
    cdata->hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, TABLESIZE, SHM_NAME);
    // we use INVALID_HANDLE_VALUE because we aren't using a file

    if (cdata->hMapFile == NULL) {
        _tprintf(TEXT("Error CreateFileMapping [%d] \n"), GetLastError());
        return FALSE;
    }

    // MapViewOfFile
    cdata->sharedTable = (SharedTable*)MapViewOfFile(cdata->hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, TABLESIZE);

    if (cdata->sharedTable == NULL) {
        _tprintf(TEXT("Error: MapViewOfFile [%d] \n"), GetLastError());
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    // Creating our mutex
    cdata->hRWMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);

    if (cdata->hRWMutex == NULL) {
        _tprintf(TEXT("Error: CreateMutex [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    cdata->hMutexEmpty = CreateMutex(NULL, FALSE, MUTEX_NAME2);

    if (cdata->hMutexEmpty == NULL) {
        _tprintf(TEXT("Error: CreateMutex [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }

    /*cdata->hMutexPipe = CreateMutex(NULL, FALSE, MUTEX_NAME3);

    if (cdata->hMutexPipe == NULL) {
        _tprintf(TEXT("Error: CreateMutex [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        return FALSE;
    }*/

    // Creating our semaphores
    cdata->hSemWrite = CreateSemaphore(NULL, 0, TAM_BUFFER, SEM_NAME_W);

    if (cdata->hSemWrite == NULL) {
        _tprintf(TEXT("Error: CreateSemaphore [%d] \n"), GetLastError());
        UnmapViewOfFile(cdata->sharedTable);
        CloseHandle(cdata->hMapFile);
        CloseHandle(cdata->hRWMutex);
        CloseHandle(cdata->hMutexEmpty);
        return FALSE;
    }

    cdata->hSemRead = CreateSemaphore(NULL, TAM_BUFFER, TAM_BUFFER, SEM_NAME_R);

    if (cdata->hSemRead == NULL) {
        _tprintf(TEXT("Error: CreateSemaphore [%d] \n"), GetLastError());
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

    // Creating our events
    cdata->printTable = CreateEventW(NULL, TRUE, FALSE, L"PrintMonitor\CSAPP");

    if (cdata->printTable == NULL)
    {
        CloseHandle(cdata->printTable);
        return -1;
    }

    cdata->suspend = CreateEventW(NULL, TRUE, TRUE, L"Suspend\CSAPP");

    if (cdata->suspend == NULL)
    {
        CloseHandle(cdata->suspend);
        return -1;
    }

    cdata->eventWritePipe = CreateEventW(NULL, TRUE, TRUE, L"WritePipe\CSAPP");

    if (cdata->eventWritePipe == NULL)
    {
        CloseHandle(cdata->eventWritePipe);
        return -1;
    }

    return TRUE;

}

void placePiece(ControlData* cdata, int cmSecondArgument, int cmThirdArgument, int cmFourthArgument) {

    TCHAR* boardAux = cdata->sharedTable->board;
    TCHAR* piece = NULL;

    switch (cmSecondArgument) { // we want to know wich piece the user placed

    case 1:
        piece = TEXT('━');
        break;
    case 2:
        piece = TEXT('┃');
        break;
    case 3:
        piece = TEXT('┏');
        break;
    case 4:
        piece = TEXT('┓');
        break;
    case 5:
        piece = TEXT('┛');
        break;
    case 6:
        piece = TEXT('┗');
        break;
    case 7:
        piece = TEXT('X');
        break;

    }

    boardAux[((cmThirdArgument - 1) * cdata->sharedTable->width) + (cmFourthArgument - 1)] = piece; // we place the piece in the auxiliar board

    CopyMemory(cdata->sharedTable->board, &boardAux, sizeof(TCHAR) - 2); // copy the board
    _tprintf(TEXT("Memory copied! [placePiece]\n"));

    return TRUE;

}

void readServerCommands(TCHAR* command, ControlData* cdata) {

    if (_tcscmp(command, TEXT("list")) == 0) {
        _tprintf(TEXT("Number of monitors: %d\n"), cdata->sharedTable->nMonitor);
    }
    else if (_tcscmp(command, TEXT("pause")) == 0) {
        ResetEvent(cdata->suspend);
        _tprintf(TEXT("Game paused!\n"));
    }
    else if (_tcscmp(command, TEXT("resume")) == 0) {
        SetEvent(cdata->suspend);
        _tprintf(TEXT("Game resumed!\n"));
    }
    else
        _tprintf(TEXT("Command unknown!\n"));

}

void readMonitorCommands(TCHAR* command, ControlData* cdata) {

    int countStrTok = 2;
    TCHAR* piece;
    TCHAR* nextToken = NULL;
    TCHAR* cmFirstArgument = NULL;
    TCHAR* cmSecondArgument = NULL;
    TCHAR* cmThirdArgument = NULL;
    TCHAR* cmFourthArgument = NULL;

    piece = wcstok_s(command, TEXT(" "), &nextToken);
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

    if (_tcscmp(cmFirstArgument, TEXT("stop")) == 0) {

        int newStop = _wtoi(cmSecondArgument) * 1000; // get the number in seconds

        CopyMemory(&cdata->sharedTable->stop, &newStop, sizeof(int));

        _tprintf(TEXT("The water flow was stoped by the player!\n"));

        SetEvent(cdata->eventWritePipe);
        //ResetEvent(cdata->eventWritePipe);
        // shows the table in the monitor but only one time
        SetEvent(cdata->printTable);
        ResetEvent(cdata->printTable);

    }
    else if (_tcscmp(cmFirstArgument, TEXT("place")) == 0) {
        placePiece(cdata, _wtoi(cmSecondArgument), _wtoi(cmThirdArgument), _wtoi(cmFourthArgument));
        _tprintf(TEXT("The player placed a piece!\n"));

        SetEvent(cdata->eventWritePipe);
        //ResetEvent(cdata->eventWritePipe);
        // shows the table in the monitor but only one time
        SetEvent(cdata->printTable);
        ResetEvent(cdata->printTable);

    }
    else if (_tcscmp(cmFirstArgument, TEXT("switch")) == 0) {

        if (!cdata->sharedTable->random) {
            _tprintf(TEXT("The player switched the game to random mode!\n"));
            BOOL falseValue = TRUE; // change the type of the game (manual/random)
            CopyMemory(&cdata->sharedTable->random, &falseValue, sizeof(BOOL));
            _tprintf(TEXT("Memory copied! [readMonitorCommands]\n"));
        }
        else {
            _tprintf(TEXT("The player switched the game to manual mode!\n"));
            BOOL falseValue = FALSE; // change the type of the game (manual/random)
            CopyMemory(&cdata->sharedTable->random, &falseValue, sizeof(BOOL));
            _tprintf(TEXT("Memory copied! [readMonitorCommands]\n"));
        }

        SetEvent(cdata->eventWritePipe);
        //ResetEvent(cdata->eventWritePipe);
        // shows the table in the monitor but only one time
        SetEvent(cdata->printTable);
        ResetEvent(cdata->printTable);


    }
    else if (_tcscmp(cmFirstArgument, TEXT("wrong")) == 0) { // we receive this when the user types one of the existing commands, but in a wrong way
        _tprintf(TEXT("The user typed a wrong command!\n"));
    }
    else
        _tprintf(TEXT("Command unknown!\n"));

}

DWORD WINAPI recieveCommand(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    SharedTable sharedTable;
    BufferCell cell;
    int count = 0;

    //while (1) {

    while (!pcd->shutdown) {

        WaitForSingleObject(pcd->hSemWrite, INFINITE); // waits if there is nothing to write on (we can't write in a used cell)

        CopyMemory(&cell, &pcd->sharedTable->command[pcd->sharedTable->posR], sizeof(BufferCell)); // we copy from the structure to the an auxiliar cell
        _tprintf(TEXT("Memory copied! [receiveCommand]\n"));

        pcd->sharedTable->posR++; // we go up in the position of the buffer
        readMonitorCommands(cell.value, pcd); // does whatever the command is suppose to do
        
        // shows the table in the Monitor.exe but only one time
        SetEvent(pcd->printTable);
        ResetEvent(pcd->printTable);
        
        if (pcd->sharedTable->posR == TAM_BUFFER)  // if we already have written in the 5 cells
            pcd->sharedTable->posR = 0; // we go to the first one again

        ReleaseSemaphore(pcd->hSemRead, 1, NULL);
        //ReleaseSemaphore(pcd->hSemEmpty, 1, NULL); // decremets the counter of the semaphore for the empty cells
        //ReleaseMutex(pcd->hMutexEmpty);

        count++;

        Sleep(1000);


    }

    //}

    return 0;

}

void createTable(ControlData* pcd, int height, int width, int timer) {

    TCHAR len = (height * width + 1) * sizeof(TCHAR);
    TCHAR* board = malloc(len);

    if (!board) {
        _tprintf(TEXT("Error alocating memory!\n"));
        CloseHandle(pcd->hMapFile);
        return;
    }

    // declare structures and initialize variables for shared memory
    SharedTable sharedTable;
    sharedTable.timer = timer;
    sharedTable.height = height;
    sharedTable.width = width;
    sharedTable.posInit = ((rand() % height + 1) * width); // creates a random initial position
    sharedTable.posEnd = ((height * (rand() % width + 1)) - 1); // creates a random final position
    sharedTable.level = 1;
    sharedTable.random = FALSE; // the game starts with the player chosing the pieces
    sharedTable.stop = 0;

    _tprintf(TEXT("posInit: %d\n"), sharedTable.posInit);
    _tprintf(TEXT("posEnd: %d\n"), sharedTable.posEnd);

    // initializing our table
    for (int i = 0; i < height * width; i++)
        if (i == sharedTable.posInit)
            _tcscpy_s(&board[i], sizeof(TCHAR), TEXT("I")); // initial position of the water
        else if (i == sharedTable.posEnd)
            _tcscpy_s(&board[i], sizeof(TCHAR), TEXT("F")); // position of the water receiver
        else
            _tcscpy_s(&board[i], sizeof(TCHAR), TEXT("."));

    _tcscpy_s(sharedTable.board, len, board);

    CopyMemory(pcd->sharedTable, &sharedTable, sizeof(SharedTable));
    _tprintf(TEXT("Memory copied! [createTable]\n"));

}

DWORD WINAPI waterFlow(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    SharedTable sharedTable;
    int waterPos = pcd->sharedTable->posInit + 1;
    int indexLastPos = pcd->sharedTable->height * pcd->sharedTable->width;
    int levelValue;
    int flow = 1; // 1 - goes right, 2 - goes left, 3 - goes down, 4 - goes up

    int drainTime = pcd->sharedTable->timer * 1000, waterTimer;

    switch (pcd->sharedTable->level) { // we only have one level atm

    case 1:
        waterTimer = 20000;
        break;

        /*case 2:
            waterTimer = 6000;
            break;

        case 3:
            waterTimer = 3000;
            break;*/

    }

    WaitForSingleObject(pcd->suspend, INFINITE); // waits if the game is suspended

    Sleep(drainTime); // waits for the time that we placed in the arguments/registry

    while (1) {

        WaitForSingleObject(pcd->suspend, INFINITE); // waits if the game is suspended

        Sleep(pcd->sharedTable->stop); // waits for the time that the user asked to

        int newStop = 0;
        CopyMemory(&pcd->sharedTable->stop, &newStop, sizeof(int)); // after waiting for that time, we put that to 0 (it can be changed by the user)

        if (pcd->sharedTable->board[waterPos] == TEXT('.')) { // if the water is in a empty space, the game ends (lost)
            _tprintf(TEXT("The water went out of the pipes! Type anything to quit.\n"));
            levelValue = -1;
            CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
            SetEvent(pcd->eventWritePipe);
            // shows the table in the monitor but only one time
            SetEvent(pcd->printTable);
            ResetEvent(pcd->printTable);
            _tprintf(TEXT("Memory copied! [waterFlow]\n"));
            return 1;
        }

        if (pcd->sharedTable->board[waterPos] == TEXT('F')) { // if the water is in the last position, the game ends (win)
            _tprintf(TEXT("The user won the game!! Type anything to quit.\n"));
            levelValue = 100;
            CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
            SetEvent(pcd->eventWritePipe);
            // shows the table in the monitor but only one time
            SetEvent(pcd->printTable);
            ResetEvent(pcd->printTable);
            _tprintf(TEXT("Memory copied! [waterFlow]\n"));
            return 1;
        }

        if (pcd->sharedTable->board[waterPos] == TEXT('X')) { // if the water is in blocked space, the game ends (lost)
            _tprintf(TEXT("The water went out of the pipes! Type anything to quit.\n"));
            levelValue = -1;
            CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
            SetEvent(pcd->eventWritePipe);
            // shows the table in the monitor but only one time
            SetEvent(pcd->printTable);
            ResetEvent(pcd->printTable);
            _tprintf(TEXT("Memory copied! [waterFlow]\n"));
            return 1;
        }

        if (waterPos - 1 >= 0 && waterPos + 1 <= indexLastPos) {

            if (pcd->sharedTable->board[waterPos] == TEXT('━')) { // the water can go left or right

                if (flow == 3 || flow == 4) { // if the water is going up or down
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 1) { // if the water is going right
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    waterPos++; // the water goes right
                }

                if (flow == 2) { // if the water is going left
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    waterPos--; // the water goes left
                }

            }
            else if (pcd->sharedTable->board[waterPos] == TEXT('┃')) {

                if (flow == 1 || flow == 2) { // if the water is going left or right
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 3) { // if the water is going down
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    waterPos += pcd->sharedTable->width; // the water goes down
                }

                if (flow == 4) { // if the water is going up
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    waterPos -= pcd->sharedTable->width; // the water goes up
                }

            }
            else if (pcd->sharedTable->board[waterPos] == TEXT('┏')) {

                if (flow == 1 || flow == 3) { // if the water is going right or down
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 2) { // if the water is going left
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 3; // change flow to down
                    waterPos += pcd->sharedTable->width; // the water goes down
                }

                if (flow == 4) { // if the water is going up
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 1; // change flow to right
                    waterPos++; // the water goes right
                }

            }
            else if (pcd->sharedTable->board[waterPos] == TEXT('┓')) {

                if (flow == 2 || flow == 3) { // if the water is going left or down
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 1) { // if the water is going right
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 3; // change flow to right
                    waterPos += pcd->sharedTable->width; // the water goes down
                }

                if (flow == 4) { // if the water is going up
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 2; // change flow to left
                    waterPos--; // the water goes left
                }

            }
            else if (pcd->sharedTable->board[waterPos] == TEXT('┛')) {

                if (flow == 2 || flow == 4) { // if the water is going left or up
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 1) { // if the water is going right
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 4; // change flow to up
                    waterPos -= pcd->sharedTable->width; // the water goes up
                }

                if (flow == 3) { // if the water is going down
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 2; // change flow to left
                    waterPos--; // the water goes left
                }

            }
            else if (pcd->sharedTable->board[waterPos] == TEXT('┗')) {

                if (flow == 1 || flow == 4) { // if the water is going right or up
                    _tprintf(TEXT("The water went out of the pipes!\n"));
                    levelValue = -1;
                    CopyMemory(&pcd->sharedTable->level, &levelValue, sizeof(int));
                    SetEvent(pcd->eventWritePipe);
                    // shows the table in the monitor but only one time
                    SetEvent(pcd->printTable);
                    ResetEvent(pcd->printTable);
                    _tprintf(TEXT("Memory copied! [waterFlow]\n"));
                    return 1;
                }

                if (flow == 2) { // if the water is going left
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 4; // change flow to up
                    waterPos -= pcd->sharedTable->width; // the water goes up
                }

                if (flow == 3) { // if the water is going down
                    pcd->sharedTable->board[waterPos] = TEXT('*'); // we place the water
                    _tprintf(TEXT("The water went through the pipe! (index: %d)\n"), waterPos);
                    flow = 1; // change flow to right
                    waterPos++; // the water goes right
                }

            }

            // the code reaches here if there is an alteration done in the table
            CopyMemory(pcd->sharedTable->board, &pcd->sharedTable->board, sizeof(pcd->sharedTable->board) - 2);
            _tprintf(TEXT("Memory copied! [waterFlow]\n"));

            // shows the table in the monitor but only one time
            SetEvent(pcd->printTable);
            ResetEvent(pcd->printTable);

            SetEvent(pcd->eventWritePipe);
            //ResetEvent(pcd->eventWritePipe);

        } // else disto?

        // depending on what level the user is playing, the water only flows again after X seconds
        Sleep(waterTimer);

    }

}

DWORD WINAPI pipeCommunicationW(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    DWORD n, res,posWrite = pcd->nClientsOnline - 1;
    OVERLAPPED ov;
    HANDLE hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);


    while (pcd->threadMustContinuePipe[posWrite] == 0) {

        //ResetEvent(pcd->eventWritePipe); // apagar
        _tprintf(_T("à espera no WAIT DO EVENT eventwritepipe\n"));
        WaitForSingleObject(pcd->eventWritePipe, INFINITE);

        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEventTemp;

        _tprintf(_T("width -> %d\n"),pcd->sharedTable->width);
        WriteFile(pcd->pipedata[posWrite].hPipe, pcd->sharedTable, sizeof(SharedTable), &n, &ov);

        WaitForSingleObject(ov.hEvent, INFINITE);

        GetOverlappedResult(pcd->pipedata[posWrite].hPipe, &ov, &n, FALSE);

        ResetEvent(pcd->eventWritePipe);

        _tprintf(_T("Enviei %d bytes!\n"), n);

    }

    return 0;

}

DWORD WINAPI pipeCommunicationR(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    TCHAR cm[commandsTAM];
    DWORD n,res,posRead=pcd->nClientsOnline-1;

    OVERLAPPED ov;
    HANDLE hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (pcd->threadMustContinuePipe[posRead] == 0) {

        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEventTemp;

        res = ReadFile(pcd->pipedata[posRead].hPipe, cm, sizeof(cm), &n, &ov);

        if (res)
            _tprintf(_T("Leitura feita de imediato \n"));

        if (!res && GetLastError() == ERROR_IO_PENDING) {
            _tprintf(_T("A aguardar pela informação.... \n"));

            WaitForSingleObject(ov.hEvent, INFINITE);
            GetOverlappedResult(pcd->pipedata[posRead].hPipe, &ov, &n, FALSE);
        }

        _tprintf(_T("Recebi o comando: '%s'.\n"),cm);

        readMonitorCommands(cm,pcd);

        /*if (_tcscmp(cm, TEXT("alone")) == 0)
            DisconnectNamedPipe(pcd->pipedata[2].hPipe);*/

    }

    _tprintf(_T("O cliente desligou-se.... \n"));

    return 0;

}

DWORD WINAPI createPipes(LPVOID p) {

    ControlData* pcd = (ControlData*)p;
    HANDLE hThread[N + 2];
    
    pcd->nClientsOnline = 0;

    for (int i = 0; i < N; i++) {

        // Creating pipes ---------
        pcd->pipedata[i].hPipe = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, PIPE_WAIT | PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            N,
            sizeof(SharedTable), // server to client
            sizeof(commandsTAM), // client to server
            1000,
            NULL
        );

        if (pcd->pipedata[i].hPipe == INVALID_HANDLE_VALUE) {
            _tprintf(TEXT("Error CreateNamedPipe %d"), GetLastError());
            return -1;
        }

        // Connect ---------
        ConnectNamedPipe(pcd->pipedata[i].hPipe, NULL); // he waits here until a new player joins
        _tprintf(TEXT("Criei o pipe e fiz o connect!\n"));

        hThread[i] = CreateThread(NULL, 0, pipeCommunicationW, pcd, 0, NULL);
        hThread[i + 2] = CreateThread(NULL, 0, pipeCommunicationR, pcd, 0, NULL);
        pcd->nClientsOnline++;

    }

    return 0;

}



int _tmain(int argc, LPTSTR argv[]) {

    srand(time(NULL));

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
#endif

    // check if the server is already running
    if (!CheckOneInstance())
        return -1;

    // check if it's being given more values than asked
    if (argc > 4) {
        _tprintf(TEXT("Error! Number of arguments have to be between 1 and 4!\n"));
        exit(1);
    }

    // declaring the variables that will be used to create the table (height x width) and start/end the game (time)
    DWORD height = 0, width = 0, timer = 0;

    // the 3 variables will be receiving data from the command line
    switch (argc)
    {
    case 2:
        height = _wtoi(argv[1]);
        width = -1;
        timer = -1;
        break;

    case 3:
        height = _wtoi(argv[1]);
        width = _wtoi(argv[2]);
        timer = -1;
        break;

    case 4:
        height = _wtoi(argv[1]);
        width = _wtoi(argv[2]);
        timer = _wtoi(argv[3]);
        break;

    default:
        break;
    }

    // verify if the values are inside the limits
    // if the varaible isn't inside the limit, it will receive -1 which means that is waiting for a new value
    // that new value can be random generated or given by the registry
    if (height < 1 || height > 20 || height == 0)
        height = -1;

    if (width < 1 || width > 20 || width == 0)
        width = -1;

    if (timer < 1 || timer == 0)
        timer = -1;

    // declaring the variables that will be used to crate the key
    HKEY key;
    DWORD answer;
    TCHAR path[TAM] = TEXT("SOFTWARE\\TP\\");

    // creating the key
    if (RegCreateKeyEx(HKEY_CURRENT_USER, path, 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &answer) != ERROR_SUCCESS) {

        DWORD error = GetLastError();
        _tprintf(TEXT("Error creating the key [%d]\n"), error);

    }
    else {

        if (answer == REG_CREATED_NEW_KEY) { // creates a key if it doesn't exist one

            _tprintf(TEXT("Key created with success!\n"));

            // check if the variables are already estabilished, or have to be given new values
            if (height == -1) {
                height = rand() % 21;
            }

            if (width == -1) {
                width = rand() % 21;
            }

            if (timer == -1) {
                timer = rand() % 26;
            }

            // declaring the variables that will bbe used to name the paths in the registry, and initializing them
            TCHAR pathHeigth[TAM] = TEXT("height");
            TCHAR pathWidth[TAM] = TEXT("width");
            TCHAR pathTime[TAM] = TEXT("timer");

            // creating an entry for the heigth, width and time in the registry
            RegSetValueEx(key, pathHeigth, 0, REG_DWORD, (LPBYTE)&height, (DWORD)(sizeof(height)));
            _tprintf(TEXT("We updated the values on registry! (height)\n"));
            RegSetValueEx(key, pathWidth, 0, REG_DWORD, (LPBYTE)&width, (DWORD)(sizeof(width)));
            _tprintf(TEXT("We updated the values on registry! (width)\n"));
            RegSetValueEx(key, pathTime, 0, REG_DWORD, (LPBYTE)&timer, (DWORD)(sizeof(timer)));
            _tprintf(TEXT("We updated the values on registry! (timer)\n"));


        }
        else if (answer == REG_OPENED_EXISTING_KEY) { // doesn't create a key when there is one already

            _tprintf(TEXT("That key already exists [%d]\n"), key);

            // declaring the variables that will bbe used to name the paths in the registry, and read/write them
            TCHAR pathHeigth[TAM] = TEXT("height");
            TCHAR pathWidth[TAM] = TEXT("width");
            TCHAR pathTime[TAM] = TEXT("timer");

            // check if the variables are already estabilished, or have to be given new values
            // if the variables have -1, we will read from the registry
            // if the variables have other values, we will write in the registry
            if (height == -1) {
                DWORD length = 8192; // LENGTH from what we read
                RegGetValue(key, NULL, pathHeigth, REG_DWORD, NULL, (LPBYTE)&height, &length);
                _tprintf(TEXT("We are using the values from the registry! (height)\n"));
            }
            else {
                RegSetValueEx(key, pathHeigth, 0, REG_DWORD, (LPBYTE)&height, (DWORD)(sizeof(height)));
                _tprintf(TEXT("We updated the values on registry! (height)\n"));
            }

            if (width == -1) {
                DWORD length = 8192; // LENGTH from what we read
                RegGetValue(key, NULL, pathWidth, REG_DWORD, NULL, (LPBYTE)&width, &length);
                _tprintf(TEXT("We are using the values from the registry! (width)\n"));
            }
            else {
                RegSetValueEx(key, pathWidth, 0, REG_DWORD, (LPBYTE)&width, (DWORD)(sizeof(width)));
                _tprintf(TEXT("We updated the values on registry! (width)\n"));
            }

            if (timer == -1) {
                DWORD length = 8192; // LENGTH from what we read
                RegGetValue(key, NULL, pathTime, REG_DWORD, NULL, (LPBYTE)&timer, &length);
                _tprintf(TEXT("We are using the values from the registry! (timer)\n"));
            }
            else {
                RegSetValueEx(key, pathTime, 0, REG_DWORD, (LPBYTE)&timer, (DWORD)(sizeof(timer)));
                _tprintf(TEXT("We updated the values on registry! (timer)\n"));
            }

        }

    }

    // final value of the 3 main variables
    _tprintf(TEXT("Height: %d\n"), height);
    _tprintf(TEXT("Width: %d\n"), width);
    _tprintf(TEXT("Timer: %d\n"), timer);

    ControlData cdata;
    HANDLE hEventTemp, hThread[2];

    // initializing shared memory
    if (!initSharedMemory(&cdata)) {
        _tprintf(TEXT("Error creating shared memory."));
        exit(1);
    }

    // create table
    createTable(&cdata, height, width, timer);

    // initializing the variables used in the semaphore

    for(int i=0;i<N;i++)
    cdata.threadMustContinuePipe[i] = 0;

    cdata.sharedTable->nServer = 0;
    cdata.sharedTable->nMonitor = 0;
    cdata.sharedTable->posW = 0;
    cdata.sharedTable->posR = 0;
    cdata.shutdown = 0;
    cdata.sharedTable->nServer++;
    cdata.id = cdata.sharedTable->nServer;

    // PIPE's (start) ------------------------------------------------------------------------------------------------------------------------------

    HANDLE hThreadTemp;

    hThreadTemp = CreateThread(NULL, 0, createPipes, &cdata, 0, NULL);

    if (hThreadTemp == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        return -1;
    }

    // PIPE's (end) ------------------------------------------------------------------------------------------------------------------------------

    // Creating our threads (game function) ---------
    hThread[0] = CreateThread(NULL, 0, recieveCommand, &cdata, 0, NULL);

    if (hThread[0] == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        WaitForSingleObject(hThread[0], INFINITE);
        return -1;
    }

    hThread[1] = CreateThread(NULL, 0, waterFlow, &cdata, 0, NULL);

    if (hThread[1] == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        cdata.shutdown = 1;
        WaitForSingleObject(hThread[1], INFINITE);
        return -1;
    }

    TCHAR quit[100];

    while (1) {

        if (WaitForSingleObject(hThread[1], 0) == WAIT_OBJECT_0) { // if the thread 1 ends, the program ends aswell
            TerminateThread(hThread[0], 0);
            return 0;
        }

        _tprintf(TEXT("The server is working. Type 'quit' lo leave leave the program or introduce a command (list, pause, resume)!\n")); // waits for a command from the ADMIN
        _getts_s(quit, 100);

        if (_tcscmp(quit, TEXT("quit")) == 0 || _tcscmp(quit, TEXT("Quit")) == 0) {
            cdata.shutdown = 1;
            int levelAux = -2;
            CopyMemory(&cdata.sharedTable->level, &levelAux, sizeof(int)); // -2 means that the game has ended because the server has left
            SetEvent(&cdata.eventWritePipe);
            _tprintf(TEXT("Memory copied! [main]\n"));
            TerminateThread(hThread[0], 0);
            TerminateThread(hThread[1], 0);
            break;
        }
        else
            if (cdata.sharedTable->level != -1 && cdata.sharedTable->level != 100) // if we lost the game/won, we dont need to read the command
                readServerCommands(&quit, &cdata); // read the command from the ADMIN

    }

    // end of server.c -----------------------------------------------------------------------------------------------------------------------------------------------

    for (int i = 0; i < N; i++)
        if (!DisconnectNamedPipe(cdata.pipedata[i].hPipe)) {
            _tprintf(_T("Error DisconnectNamedPipe"));
            return -1;
        }

    for (int i = 0; i < N; i++)
        cdata.threadMustContinuePipe[i] = 1;

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
    return 0;

}