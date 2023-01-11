#pragma once

#include <Windows.h>
#include <tchar.h>
#include <Windows.h>
#include <tchar.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include <synchapi.h>

#define TAM 803
#define commandsTAM 100
#define SHM_NAME TEXT("sharedMemory")
#define MUTEX_NAME _T("RWMUTEX")
#define MUTEX_NAME2 _T("MUTEXSEMAPHORE")
#define MUTEX_NAME3 _T("MUTEXPIPE")
#define EVENT_NAME _T("NEWTABLE")
#define SEM_NAME_W _T("SemaphoreWrite")
#define SEM_NAME_R _T("SemaphoreRead")
#define SEM_NAME_E _T("SemaphoreEmpty")
#define TAM_BUFFER 5
#define PIPE_NAME _T("\\\\.\\pipe\\CliSer")
#define N 2

typedef struct {
    HANDLE hPipe; // pipe for the comunication client - server
    //BOOL state;
    //OVERLAPPED ov;
}PipeData;

typedef struct {
    int id;
    TCHAR value[TAM];
}BufferCell;

typedef struct _Table {
    int timer; // time until the water starts flowing
    int height; // number of lines in the board
    int width; // number of collums in the board
    TCHAR board[TAM]; // board itself
    int posInit; // index of the position that the water starts to flow
    int posEnd; // index of the position that we have to take the water
    BufferCell command[TAM_BUFFER]; // commands that get values from the monitor (circular buffer)
    int nServer; // number of servers online
    int nMonitor; // number of monitors online
    int posW; // position of the circular buffer (writing)
    int posR; // position of the circular buffer (reading)
    int level; // level of the game (-1: lost / 100: win / 1: level 1 / 2: level 2 / 3: level 3 (...))
    int stop; // ammount of seconds that the water is stoped when the user asks to
    BOOL random; // used to check if the game is giving random pieces or the user is chosing the pieces
}SharedTable;
#define TABLESIZE sizeof(SharedTable)

typedef struct _ControlData {
    HANDLE hMapFile;
    SharedTable* sharedTable; // shared structure
    HANDLE hRWMutex; // mutex to control the number of monitors
    HANDLE printTable; // used for an event (controling the amount of prints of the table)
    HANDLE hSemWrite; // handle for the semaphore (controling the writing)
    HANDLE hSemRead; // handle for the semaphore (controling the reading)
    HANDLE hMutexEmpty; // handle for the semaphore (waits if there is an empty cell)
    HANDLE suspend; // used for an event (controling the pause/resume command)
    HANDLE monitorLife; // not finished - 15 seconds of life from the monitor
    int shutdown; // flag to shutdown the circular buffer
    int id; // id from the monitor that is writing in the circular buffer
    int threadMustContinue; // flag to stop the threads in monitor.c
    int  threadMustContinuePipe[N]; // flag to stop one thread in server.c
    PipeData pipedata[N]; // reference to the stuct that has all the information about our pipe
    HANDLE hEvents[N];
    int nClientsOnline;
    HANDLE eventWritePipe;
}ControlData;

/*typedef struct _ThreadData { // structure used only to send a group of variables to a thread
    PipeData pdata[N];
    SharedTable sTable;
}ThreadData;*/