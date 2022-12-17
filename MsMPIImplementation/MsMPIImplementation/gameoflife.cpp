#include "gameoflife.h"
#include <mpi.h>
#include <cstring>
#include <cmath>
#include <exception>
#include <windows.h>
#include <cstdio>

typedef char boardCelType;

static boardCelType* board, *temp, *bufY, *bufX, *bufT1X, *bufT1Y, * bufT2X, * bufT2Y;
static int sizeX, sizeY, masterX;
static int this_id, no_proc, procSqrt, peers[8];
static size_t board_size;

/* peers  -- trigonometric order
 * 5 1 4
 * 2 c 0
 * 6 3 7
 */

void init_gof(int x, int y, int no_proc, int procID) {
    procSqrt = (int)sqrt(no_proc);

    if (!procID && (procSqrt * procSqrt != no_proc || x % no_proc != 0 || y % no_proc != 0)) { // in master process, we verify the values
        throw std::exception("invalid dimensions");
    }
    this_id = procID;
    ::no_proc = no_proc;

    masterX = x + 2;

	sizeX = x / procSqrt + 2;
	sizeY = y / procSqrt + 2;

    board_size = sizeY * sizeX * sizeof(boardCelType);

    //init boards
	board = new char[board_size]; // +2 meaning the frame
    temp = new char[board_size]; // +2 meaning the frame

    memset(board, 0, board_size);
    memset(temp,  0, board_size);

    //init buffer
    bufY   = new char[sizeY - 2];
    bufT1Y = new char[sizeY - 2];
    bufT2Y = new char[sizeY - 2];

    bufX   = new char[sizeX - 2];
    bufT1X = new char[sizeX - 2];
    bufT2X = new char[sizeX - 2];


    //init peers
    peers[0] = procID + 1;
    peers[2] = procID - 1;

    peers[1] = procID - procSqrt;
    peers[3] = procID + procSqrt;

    //verify peers
    if (peers[1] < 0)        peers[1] = -1;
    if (peers[3] >= no_proc) peers[3] = -1;

    if (procID % procSqrt == 0)            peers[2] = -1;
    if (procID % procSqrt == procSqrt - 1) peers[0] = -1;

    //init corner peers
    
    //4
    if (peers[0] != -1 && peers[1] != -1)
        peers[4] = peers[1] + 1;
    else
        peers[4] = -1;

    //5
    if (peers[1] != -1 && peers[2] != -1)
        peers[5] = peers[1] - 1;
    else
        peers[5] = -1;

    //6
    if (peers[2] != -1 && peers[3] != -1)
        peers[6] = peers[3] - 1;
    else
        peers[6] = -1;

    //7
    if (peers[0] != -1 && peers[3] != -1)
        peers[7] = peers[3] + 1;
    else
        peers[7] = -1;
}

static void set_board(char* value) {
    int i = 0; 
    // -1 because of the padding
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++)
            board[y * sizeX + x] = value[i++];

    /*char* c = new char[board_size +1];
    i = 0;
    for (int y = 0; y < sizeY; y++) {
        for (int x = 0; x < sizeX; x++)
            c[i++] = (char)(board[y*sizeX + x] + 32);
        c[i++] = '\n';
    }
    c[i] = '\0';
    printf("process %d : \n%s", this_id ,c);
    fflush(stdout);
    delete[] c;*/
}

static char compute(int y, int x) {
    unsigned sum =
            ( board[(y - 1) * sizeX + (x - 1)]
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
    memset(buffer, 0, buffsize);

    MPI_Status s;

    MPI_Recv(buffer, buffsize, MPI_CHAR, 0, read_tag, MPI_COMM_WORLD, &s);

    set_board(buffer);

    delete[] buffer;
}

void master_proc_init(char* boardt, int sX, int sY) {
    char** buffer = new char* [no_proc];

    /* (p%n) * (boardActualX/n) to determin the offset of the column. 
     * (p/n) * (boardActualY/n) to determin the offset of the row
     * n - total number of processes
     * p - curent process id
     * boardActualX, boardActualY - size of the actual rows and column, without the padding
     */
    const int buffsize = (sizeY - 2) * (sizeX - 2);
    MPI_Request *procStatus = new MPI_Request[no_proc];

    for (int p = no_proc - 1; p >= 0; p--) {
        buffer[p] = new char[buffsize];
        memset(buffer[p], 0, buffsize);
        auto pdnby = (p / procSqrt) * (sizeY - 2);
        auto pmnbx = (p % procSqrt) * (sizeX - 2);
        //-2 because of the padding
        int k = 0;
        for (int y = 1; y < sizeY - 1; y++) {
            for (int x = 1; x < sizeX - 1; x++) {
                auto _X = x + pmnbx;
                auto _Y = y + pdnby;
                buffer[p][k++] = boardt[_Y * sX + _X];
            }
        }

        /*char* c = new char[board_size + 1];
        int i = 0;
        k = 0;
        for (int y = 1; y < sizeY - 1; y++) {
            for (int x = 1; x < sizeX - 1; x++)
                c[i++] = (char)(buffer[p][k++] + 32);
            c[i++] = '\n';
        }
        c[i] = '\0';
        printf("to send to process %d : \n%s", p, c);
        fflush(stdout);
        delete[] c;*/

        if (p != 0) {
            MPI_Isend(buffer[p], buffsize, MPI_CHAR, p, read_tag, MPI_COMM_WORLD, &procStatus[p]);
        }
        else {
            set_board(buffer[0]);
        }
    }

    Sleep(1000);

    MPI_Status _;
    for (int p = no_proc - 1; p > 0; p--) {
        MPI_Wait(&procStatus[p], &_);
        delete[] buffer[p];
    }

    delete[] buffer[0];

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
        auto pdnby = (p / procSqrt) * (sizeY - 2);
        auto pmnbx = (p % procSqrt) * (sizeX - 2);

        MPI_Status tmp;
        MPI_Recv(buffer, buffsize, MPI_CHAR, p, 2, MPI_COMM_WORLD, &tmp);

        //-2 because of the padding
        int k = 0;
        for (int y = 1; y < sizeY - 1; y++) {
            for (int x = 1; x < sizeX - 1; x++) {
                auto _X = x + pmnbx;
                auto _Y = y + pdnby;
                boardtd[_Y * masterX + _X] = buffer[k++];
            }
        }

    }

    // for self
    for (int y = 1; y < sizeY - 1; y++)
        for (int x = 1; x < sizeX - 1; x++)
            boardtd[y * masterX + x] = board[y * sizeX + x];
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
    static MPI_Request req[8];
    static int corners[8];
    static int call = 0;
    MPI_Status s;

    //read up peer
    if (peers[1] != -1) { //up padding, row 0
        if (call) MPI_Wait(&req[1], &s);
        for (int x = 1; x < sizeX - 1; x++)
            bufT1X[x - 1] = board[sizeX + x];

        MPI_Isend(bufT1X, sizeX - 2, MPI_CHAR, peers[1], 0, MPI_COMM_WORLD, &req[1]);
        MPI_Recv(bufX, sizeX - 2, MPI_CHAR, peers[1], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int x = 1; x < sizeX - 1; x++)
            board[x] = bufX[x - 1];
    }

    //read down peer
    if (peers[3] != -1) { //down padding, last row sizeY -1 
        if (call) MPI_Wait(&req[3], &s);
        for (int x = 1; x < sizeX - 1; x++)
            bufT2X[x - 1] = board[(sizeY - 2) * sizeX + x];

        MPI_Isend(bufT2X, sizeX - 2, MPI_CHAR, peers[3], 0, MPI_COMM_WORLD, &req[3]);
        MPI_Recv(bufX, sizeX - 2, MPI_CHAR, peers[3], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int x = 1; x < sizeX - 1; x++)
            board[(sizeY - 1) * sizeX + x] = bufX[x - 1];
    }

    //read left peer
    if (peers[2] != -1) { //left padding, column 0
        if (call) MPI_Wait(&req[2], &s);
        for (int y = 1; y < sizeY - 1; y++)
            bufT1Y[y - 1] = board[y * sizeX + 1];

        MPI_Isend(bufT1Y, sizeY - 2, MPI_CHAR, peers[2], 0, MPI_COMM_WORLD, &req[2]);
        MPI_Recv(bufY, sizeY - 2, MPI_CHAR, peers[2], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int y = 1; y < sizeY - 1; y++)
            board[y * sizeX] = bufY[y - 1];
    }

    //read right peer
    if (peers[0] != -1) { //right padding, last column sizeX - 1
        if (call) MPI_Wait(&req[0], &s);
        for (int y = 1; y < sizeY - 1; y++)
            bufT2Y[y - 1] = board[y * sizeX + (sizeX - 2)];

        MPI_Isend(bufT2Y, sizeY - 2, MPI_CHAR, peers[0], 0, MPI_COMM_WORLD, &req[0]);
        MPI_Recv(bufY, sizeY - 2, MPI_CHAR, peers[0], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        for (int y = 1; y < sizeY - 1; y++)
            board[y * sizeX + (sizeX - 1)] = bufY[y - 1];
    }

    //call corners
    if (peers[4] != -1) {
        if (call) MPI_Wait(&req[4], &s);
        corners[4] = board[sizeX + (sizeX - 2)];
        MPI_Isend(&corners[4], 1, MPI_CHAR, peers[4], 0, MPI_COMM_WORLD, &req[4]);
        MPI_Recv(&corners[4 - 4], 1, MPI_CHAR, peers[4], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        board[0 + (sizeX - 1)] = corners[4 - 4];
    }

    if (peers[5] != -1) {
        if (call) MPI_Wait(&req[5], &s);
        corners[5] = board[sizeX + 1];
        MPI_Isend(&corners[5], 1, MPI_CHAR, peers[5], 0, MPI_COMM_WORLD, &req[5]);
        MPI_Recv(&corners[5 - 4], 1, MPI_CHAR, peers[5], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        board[0 + 0] = corners[5 - 4];
    }

    if (peers[6] != -1) {
        if (call) MPI_Wait(&req[6], &s);
        corners[6] = board[(sizeY - 2) * sizeX + 1];
        MPI_Isend(&corners[6], 1, MPI_CHAR, peers[6], 0, MPI_COMM_WORLD, &req[6]);
        MPI_Recv(&corners[6 - 4], 1, MPI_CHAR, peers[6], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        board[(sizeY - 1) * sizeX + 0] = corners[6 - 4];
    }

    if (peers[7] != -1) {
        if (call) MPI_Wait(&req[7], &s);
        corners[7] = board[(sizeY - 2) * sizeX + sizeX - 2];
        MPI_Isend(&corners[7], 1, MPI_CHAR, peers[7], 0, MPI_COMM_WORLD, &req[7]);
        MPI_Recv(&corners[7 - 4], 1, MPI_CHAR, peers[7], MPI_ANY_TAG, MPI_COMM_WORLD, &stat);
        board[(sizeY - 1) * sizeX + sizeX - 1] = corners[7 - 4];
    }

    call = 1;
}