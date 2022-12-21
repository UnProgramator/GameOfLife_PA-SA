#include <mpi.h>
#include <cstdio>
#include <cstring>

#include <iostream>
#include <windows.h>

#include "gameoflife.h"

/* IMPORTANT:
 * if true, then the scene will have the dimension 40x80, will compute 100 generations and will use the simple board implementation and will display
 * otherwise, the board can have another size which do not affect the display in console and will not be printed. This is the implemmentation which
 *			can be used for measurements, as the other depends on the writing to console speed
 */

#define use_display false

#if use_display == true

#define boardx_actual 80
#define boardy_actual 48

#define generations 100

#else

#define boardx_actual 2100
#define boardy_actual 960

#define generations 1000

#endif

#define boardx (boardx_actual + 2)
#define boardy (boardy_actual + 2)

#define initTimmer(t) t = omp_get_wtime()
#define computeTimmer(t) t = omp_get_wtime() - t

void simple_board(char board[boardy][boardx], int deltaY = 0, int deltaX = 0) { //board for test purpose, predefined pattern
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

void random_board(char board[boardy][boardx]) { // random board layout
    int x, y;
    for (int i = 0; i < boardx_actual * boardy_actual / 10; i++) {
        x = rand() % boardx_actual + 1;
        y = rand() % boardy_actual + 1;
        board[y][x] = 1;
    }
}

char board[boardy][boardx];

int main(int argc, char** argv) {
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the rank of the process
    int my_rank, npere;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &npere);

    if (my_rank == 0) {//the main one
        double time, time2, timef;

        time = MPI_Wtime();

        init_gof(boardx_actual, boardy_actual, npere, my_rank);

        memset(board, 0, boardy * boardx * sizeof(board[0][0]));

#if use_display == true
        simple_board(board);
        simple_board(board, 10, 30);
        simple_board(board, 40, 50);
        simple_board(board, 43, 40);
        simple_board(board, -4, 3);
#else
        random_board(board);
#endif

        master_proc_init((char*)board, boardx, boardy);

#if use_display == true
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
#endif

        time2 = MPI_Wtime();

        for (int i = 0; i < generations; i++) {
            step(false);

#if use_display == true
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
#endif
        }
        int x;
        MPI_Status s;
        for (int i = 1; i < npere; i++) {
            MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &s);
        }

        timef = MPI_Wtime();
        printf("---------!!!!!!!!!!!!!!!---------- the time %lf seconds, with init was %lf seconds", timef - time2, timef - time);
        fflush(stdout);
    }
    else {

        init_gof(boardx_actual, boardy_actual, npere, my_rank);

        proc_init();

        if (use_display)
            retDisplay();

        for (int i = 0; i < generations; i++)
            step(use_display);
        int x = 1;
        MPI_Send(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
}

//int main(int argc, char** argv) {
//    // Initialize the MPI environment
//    MPI_Init(&argc, &argv);
//
//    // Get the rank of the process
//    int my_rank, npere, peers[4] = {-1, -1, -1, -1};
//    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
//    MPI_Comm_size(MPI_COMM_WORLD, &npere);
//
//    if (my_rank == 0) {//the main one
//        double time, time2, timef;
//        
//        time = MPI_Wtime();
//        
//        init_gof(boardx_actual, boardy_actual, npere, my_rank);
//
//        memset(board, 0, boardy * boardx * sizeof(board[0][0]));
//
//#if use_display == true
//        simple_board(board);
//        simple_board(board, boardy/2 - 4, -1);
//        simple_board(board, 10, 20);
//#else 
//        random_board(board);
//#endif
//
//         master_proc_init((char*)board, boardx, boardy);
//
//#if use_display == true
//        (std::cout << "Starting draw\n").flush();
//        memset(board, 0, boardy * boardx);
//        get_display_board((char*)board);
//        std::cout << "---------------------------------------------\n";
//        for (int y = 1; y <= boardy_actual; y++) {
//            std::cout << "|";
//            for (int x = 1; x <= boardx_actual; x++)
//                std::cout << (char)(board[y][x] + 32);
//            std::cout << "|\n";
//        }
//        std::cout << "---------------------------------------------\n";
//        std::cout.flush();
//#endif
//
//        time2 = MPI_Wtime();
//
//        for (int i = 0; i < generations; i++) {
//            step(false);
//
//#if use_display == true
//            (std::cout << "Starting draw\n").flush();
//            memset(board, 0, boardy * boardx);
//            get_display_board((char*)board);
//            std::cout << "---------------------------------------------\n";
//            for (int y = 1; y <= boardy_actual; y++) {
//                std::cout << "|";
//                for (int x = 1; x <= boardx_actual; x++)
//                    std::cout << (char)(board[y][x] + 32);
//                std::cout << "|\n";
//            }
//            std::cout << "---------------------------------------------\n";
//            std::cout.flush();
//            Sleep(1000);
//#endif
//        }
//
//        int x;
//        MPI_Status s;
//        for (int i = 1; i < npere; i++) {
//            MPI_Recv(&x, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &s);
//        }
//
//        timef = MPI_Wtime();
//        printf("---------!!!!!!!!!!!!!!!---------- the time %lf seconds, with init was %lf seconds", timef - time2, timef - time);
//    }
//    else {
//
//        init_gof(boardx_actual, boardy_actual, npere, my_rank);
//
//        proc_init();
//
//        if (use_display)
//            retDisplay();
//
//        for(int i = 0; i < generations; i++)
//            step(use_display);
//        int x = 1;
//        MPI_Send(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
//    }
//
//    MPI_Finalize();
//}