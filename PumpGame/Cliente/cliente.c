#include "server.h"

DWORD WINAPI readPipe(LPVOID p) {

    HANDLE hPipe = (HANDLE)p;
    SharedTable stable;
    DWORD n, res;

    OVERLAPPED ov;
    HANDLE hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (1) {

        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEventTemp;

        res = ReadFile(hPipe, &stable, sizeof(stable), &n, &ov);

        if(res)
            _tprintf(_T("Leitura feita de imediato \n"));
        if (!res && GetLastError() == ERROR_IO_PENDING) {
            _tprintf(_T("A aguardar pela informação.... \n"));

            WaitForSingleObject(ov.hEvent, INFINITE);
            GetOverlappedResult(hPipe, &ov, &n, FALSE);
        }

        if (n==0) {
            _tprintf(TEXT("The server closed! Type quit.\n"));
            break;
        }

        if (stable.level == -1) {
            _tprintf(TEXT("The water went out of the pipes! Type quit.\n"));
            break;
        }
        else if (stable.level == 100) {
            _tprintf(TEXT("You won the game! Type quit.\n"));
            break;
        }
        else if (stable.level == -2) {
            _tprintf(TEXT("The server closed! Type quit.\n"));
            break;
        }

        int nlinha = 1;
        int ncoluna = 1;

        _tprintf(TEXT("\n   "));

        for (int i = 0; i < stable.width; i++) {

            _tprintf(TEXT("%02d "), ncoluna);
            ncoluna++;

        }

        _tprintf(TEXT("\n"));

        for (int i = 0; i < stable.height; i++) {

            _tprintf(TEXT("%02d "), nlinha);

            for (int j = 0; j < stable.width; j++) {

                _tprintf(TEXT("%2c "), stable.board[(i * stable.width) + j]);

            }
            _tprintf(TEXT("\n"));
            nlinha++;
        }

        _tprintf(TEXT("\n\n"));

        if (!stable.random) { // only show the pieces when the game is in manual mode
            _tprintf(TEXT("Piece 1: ━\n"));
            _tprintf(TEXT("Piece 2: ┃\n"));
            _tprintf(TEXT("Piece 3: ┏\n"));
            _tprintf(TEXT("Piece 4: ┓\n"));
            _tprintf(TEXT("Piece 5: ┛\n"));
            _tprintf(TEXT("Piece 6: ┗\n"));
            _tprintf(TEXT("Piece 7: X\n"));
        }

    }

    return 0;

}

int _tmain(int argc, LPTSTR argv[]) {

    srand(time(NULL));
    HANDLE hPipe;
    TCHAR cm[commandsTAM];
    DWORD n;

#ifdef UNICODE
    _setmode(_fileno(stdin), _O_WTEXT);
    _setmode(_fileno(stdout), _O_WTEXT);
#endif

    hPipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

    if (hPipe == NULL) {
        _tprintf(TEXT("Error CreateFile\n"));
        return -1;
    }

    /*if (!WaitNamedPipe(PIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT)) {
        _tprintf(TEXT("O pipe não existe\n"));
        return -1;
    }*/


    HANDLE hThread;
    DWORD nBytes, res;

    hThread = CreateThread(NULL, 0, readPipe, (LPVOID)hPipe, 0, &nBytes);

    if (hThread == NULL) {
        _tprintf(TEXT("\nError CreateThread!\n"));
        return -1;
    }


    OVERLAPPED ov;
    HANDLE hEventTemp = CreateEvent(NULL, TRUE, FALSE, NULL);

    do {

        if (_fgetts(cm, commandsTAM, stdin) == NULL)
            break;

        cm[_tcslen(cm) - 1] = '\0';

        ZeroMemory(&ov, sizeof(ov));
        ov.hEvent = hEventTemp;
        
        res = WriteFile(hPipe, cm, ((DWORD) _tcslen(cm)+1)*sizeof(TCHAR), &n, &ov);

        if (res)
            _tprintf(_T("Leitura feita de imediato \n"));
        if (!res && GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(ov.hEvent, INFINITE);
            GetOverlappedResult(hPipe, &ov, &n, FALSE);
        }

        _tprintf(_T("Enviei %d bytes '%s'!\n"), n,cm);

    } while (_tcscmp(cm, TEXT("quit")) != 0);

    return 0;

}