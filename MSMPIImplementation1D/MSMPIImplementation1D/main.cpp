#include <mpi.h>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <windows.h>

#include "gameoflife.h"

#define use_display true

#if use_display == true

#define boardx_actual 80
#define boardy_actual 80

#else

#define boardx_actual 80
#define boardy_actual 48

#endif

#define boardx (boardx_actual + 2)
#define boardy (boardy_actual + 2)



#define generations 100000

#define initTimmer(t) t = omp_get_wtime()
#define computeTimmer(t) t = omp_get_wtime() - t

void simple_board(char board[boardy][boardx], int deltaY = 0, int deltaX = 0) {
    board[11 + deltaY][10 + deltaX] = board[11 + deltaY][11 + deltaX] = board[11 + deltaY][12 + deltaX] = 1;
    board[12 + deltaY][11 + deltaX] = 1;
    board[13 + deltaY][11 + deltaX] = 1;
    board[14 + deltaY][10 + deltaX] = board[14 + deltaY][11 + deltaX] = board[14 + deltaY][12 + deltaX] = 1;

    board[16 + deltaY][10 + deltaX] = board[16 + deltaY][11 + deltaX] = board[16 + deltaY][12 + deltaX] = 1;
    board[17 + deltaY][10 + deltaX] = board[17 + deltaY][11 + deltaX] = board[17 + deltaY][12 + deltaX] = 1;

    board[19 + deltaY][10 + deltaX] = board[19 + deltaY][11 + deltaX] = board[19 + deltaY][12 + deltaX] = 1;
    board[20 + deltaY][11 + deltaX] = 1;
    board[21 + deltaY][11 + deltaX] = 1;
    board[22 + deltaY][10 + deltaX] = board[22 + deltaY][11 + deltaX] = board[22 + deltaY][12 + deltaX] = 1;


    board[11 + deltaY][41 + deltaX] = board[11 + deltaY][42 + deltaX] = board[12 + deltaY][41 + deltaX] = 1;
    board[14 + deltaY][43 + deltaX] = board[14 + deltaY][44 + deltaX] = board[13 + deltaY][44 + deltaX] = 1;

    board[11 + deltaY][30 + deltaX] = board[11 + deltaY][31 + deltaX] = board[11 + deltaY][32 + deltaX] = board[11 + deltaY][33 + deltaX] = 1;
}

inline void gotoxy(short col, short line) {
    SetConsoleCursorPosition(
        GetStdHandle(STD_OUTPUT_HANDLE),
        COORD{ col, line }
    );
}

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the rank of the process
    int my_rank, npere, peers[4] = { -1, -1, -1, -1 };
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &npere);


    init_gof(boardx_actual, boardy_actual, npere, my_rank);

    printf("Process %d initialized gof successfully\n", my_rank);
    fflush(stdout);

    if (my_rank == 0) {//the main one
        char board[boardy][boardx];
        memset(board, 0, boardy * boardx * sizeof(board[0][0]));
        simple_board(board);
        simple_board(board, 10, 30);
        simple_board(board,40, 50);
        simple_board(board,43, 40);
        simple_board(board,-4, 3);

        master_proc_init((char*)board);

        if (use_display) {
            memset(board, 0, boardy * boardx);
            get_display_board((char*)board);
            std::cout << "---------------------------------------------\n";
            for (int y = 1; y <= boardy_actual; y++) {
                std::cout << "|";
                for (int x = 1; x <= boardx_actual; x++)
                    std::cout << (char)(board[y][x] + 32);
                std::cout << "|\n";
            }
            std::cout << "---------------------------------------------\n";
            std::cout.flush();
            Sleep(1000);
        }

        for (int i = 0; i < 100; i++) {
            step(false);

            if (use_display) {
                (std::cout << "Starting draw\n").flush();
                memset(board, 0, boardy * boardx);
                get_display_board((char*)board);
                std::cout << "---------------------------------------------\n";
                for (int y = 1; y <= boardy_actual; y++) {
                    std::cout << "|";
                    for (int x = 1; x <= boardx_actual; x++)
                        std::cout << (char)(board[y][x] + 32);
                    std::cout << "|\n";
                }
                std::cout << "---------------------------------------------\n";
                std::cout.flush();
                Sleep(1000);
            }
        }
    }
    else {
        proc_init();

        if (use_display)
            retDisplay();

        for (int i = 0; i < 100; i++)
            step(use_display);
    }

    MPI_Finalize();
}