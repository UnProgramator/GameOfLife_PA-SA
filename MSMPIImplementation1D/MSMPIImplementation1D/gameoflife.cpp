#include "gameoflife.h"
#include <mpi.h>
#include <cstring>
#include <cmath>
#include <exception>
#include <windows.h>
#include <cstdio>

typedef char boardCelType;

static boardCelType* board, * temp, * bufY, * bufX, * bufT1X, * bufT1Y, * bufT2X, * bufT2Y;
static int sizeX, sizeY;
static int this_id, no_proc, peers[2];
static size_t board_size;

/* peers  -- trigonometric order
 * 5 1 4
 * 2 c 0
 * 6 3 7
 */

void init_gof(int x, int y, int no_proc, int procID) {

    if (!procID && x % no_proc != 0 ) { // in master process, we verify the values
        throw std::exception("invalid dimensions");
    }
    this_id = procID;
    ::no_proc = no_proc;

    sizeX = x + 2;
    sizeY = y / no_proc  + 2;

    board_size = sizeY * sizeX * sizeof(boardCelType);

    //init boards
    board = new char[board_size]; // +2 meaning the frame
    temp = new char[board_size]; // +2 meaning the frame

    memset(board, 0, board_size);

    //init buffer
    bufX = new char[sizeX - 2];
    bufT1X = new char[sizeX - 2];
    bufT2X = new char[sizeX - 2];


    //init peers
    if (procID > 0)
        peers[0] = procID - 1;
    else
        peers[0] = -1;

    if (procID + 1 < no_proc)
        peers[1] = procID + 1;
    else
        peers[1] = -1;
}

static void set_board(char* value) {
    int i = 0;
    // -1 because of the padding
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++)
            board[y * sizeX + x] = value[i++];
}

static char compute(int y, int x) {
    unsigned sum =
        (board[(y - 1) * sizeX + (x - 1)]
            + board[(y - 1) * sizeX + x]
            + board[(y - 1) * sizeX + (x + 1)]
            + board[y * sizeX + (x - 1)]
            + board[y * sizeX + (x + 1)]
            + board[(y + 1) * sizeX + (x - 1)]
            + board[(y + 1) * sizeX + x]
            + board[(y + 1) * sizeX + (x + 1)]);
    return (board[y * sizeX + x] == 1 && sum == 2) || sum == 3;
}



static inline void compute_generation() {
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++) {
            temp[y * sizeX + x] = compute(y, x);
        }
}

static void communicateWithPears();

void step(bool displayOn) {
    memcpy(temp, board, board_size);
    communicateWithPears();
    compute_generation();
    memcpy(board, temp, board_size);
    if (displayOn)
        retDisplay();
}

#define read_tag 192

void proc_init() {
    const int buffsize = (sizeY - 2) * (sizeX - 2);
    char* buffer = new char[buffsize];

    MPI_Status s;

    MPI_Recv(buffer, buffsize, MPI_CHAR, 0, read_tag, MPI_COMM_WORLD, &s);

    set_board(buffer);

    delete[] buffer;
}

void master_proc_init(char* boardt) {
    char** buffer = new char* [no_proc];

    /* 
     * p * (boardActualY/n) to determin the offset of the row - Y axis
     * sizeY = actualBoardsY/n + 2
     * n - total number of processes
     * p - curent process id
     * boardActualX, boardActualY - number of the rows and columns of the entire matrix (not the local copy), without the padding
     */
    const int buffsize = (sizeY - 2) * (sizeX - 2);
    MPI_Request* procStatus = new MPI_Request[no_proc];
    for (int p = no_proc - 1; p >= 0; p--) {
        buffer[p] = new char[buffsize];
        auto pdnby = p * (sizeY - 2);
        //-2 because of the padding
        int k = 0;
        for (int y = 1; y < sizeY - 1; y++) {
            for (int x = 1; x < sizeX - 1; x++) {
                auto Y = y + pdnby;
                buffer[p][k++] = boardt[Y * sizeX + x];
            }
        }

        if (p != 0) {
            MPI_Isend(buffer[p], buffsize, MPI_CHAR, p, read_tag, MPI_COMM_WORLD, &procStatus[p]);
        }
        else {
            set_board(buffer[0]);
        }
    }

    MPI_Status _;
    for (int p = no_proc - 1; p > 0; p--)
        MPI_Wait(&procStatus[p], &_);

    for (int i = 0; i < no_proc; i++) {
        delete[] buffer[i];
    }

    delete[] buffer;
}

void get_display_board(char* boardtd) {
    static char* buffer = nullptr;
    const int buffsize = (sizeY - 2) * (sizeX - 2);
    if (buffer == nullptr) {
        buffer = new char[buffsize];
    }

    //for other processes
    for (int p = 1; p < no_proc; p++) {
        auto pdnby = p * (sizeY - 2);

        MPI_Status tmp;
        MPI_Recv(buffer, buffsize, MPI_CHAR, p, 2, MPI_COMM_WORLD, &tmp);

        //-2 because of the padding
        int k = 0;
        for (int y = 1; y < sizeY - 1; y++) {
            for (int x = 1; x < sizeX - 1; x++) {
                auto _Y = y + pdnby;
                boardtd[_Y * sizeX + x] = buffer[k++];
            }
        }
    }

    // for self
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++)
            boardtd[y * sizeX + x] = board[y * sizeX + x];
}

void retDisplay() {
    static int buffsize = (sizeY - 2) * (sizeX - 2);
    static char* buf = new char[buffsize];

    int k = 0;
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++) {
            buf[k] = board[y * sizeX + x];
            k++;
        }

    MPI_Send(buf, buffsize, MPI_CHAR, 0, 2, MPI_COMM_WORLD);
}


static void communicateWithPears() {
    MPI_Status stat;
    static MPI_Request req[2];
    static int call = 0;
    MPI_Status s;

    //read up peer
    if (peers[0] != -1) { //up padding, send row 1 and receive row 0
        if (call) MPI_Wait(&req[0], &s);
        for (int x = 1; x < sizeX - 1; x++)
            bufT1X[x - 1] = board[sizeX + x];

        MPI_Isend(bufT1X, sizeX - 2, MPI_CHAR, peers[0], 0, MPI_COMM_WORLD, &req[0]);
        
    }

    if (peers[1] != -1) { //up padding, send row y-2 and receive row y-1

        if (call) MPI_Wait(&req[1], &s);
        for (int x = 1; x < sizeX - 1; x++)
            bufT2X[x - 1] = board[(sizeY - 2) * sizeX + x];

        MPI_Isend(bufT2X, sizeX - 2, MPI_CHAR, peers[1], 0, MPI_COMM_WORLD, &req[1]);
       
    }

    if (peers[0] != -1) {
        MPI_Recv(bufX, sizeX - 2, MPI_CHAR, peers[0], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int x = 1; x < sizeX - 1; x++)
            board[x] = bufX[x - 1];
    }

    if (peers[1] != -1) {
        MPI_Recv(bufX, sizeX - 2, MPI_CHAR, peers[1], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int x = 1; x < sizeX - 1; x++)
            board[(sizeY - 1) * sizeX + x] = bufX[x - 1];
    }

    call = 1;
}