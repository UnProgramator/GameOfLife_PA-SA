#include <iostream>
#include <windows.h>
#include <omp.h>

/* IMPORTANT:
 * if true, then the scene will have the dimension 40x80, will compute 100 generations and will use the simple board implementation and will display
 * otherwise, the board can have another size which do not affect the display in console and will not be printed. This is the implemmentation which
 *			can be used for measurements, as the other depends on the writing to console speed
 */

#define use_display false

#if use_display == true

#define boardx_actual 80
#define boardy_actual 40
#define generations 100

#else

#define boardx_actual 960
#define boardy_actual 2100
#define generations 1000

#endif

// I put a border around the actual used part of the matrix, for easier computation

#define boardx (boardx_actual + 2)
#define boardy (boardy_actual + 2)

//macros which make the time measurement easier
#define initTimmer(t) t = omp_get_wtime()
#define computeTimmer(t) t = omp_get_wtime() - t

//boards to be used in computation
char temp[boardy][boardx], board[boardy][boardx];

size_t board_mem_size = boardx * boardy * sizeof(temp[0][0]);

//use for display only
inline void gotoxy(short col, short line) {
	SetConsoleCursorPosition(
		GetStdHandle(STD_OUTPUT_HANDLE),
		COORD{ col, line }
	);
}

inline char compute(int i, int j) { // compute neighboars of a cell
	unsigned sum =
		(	  board[i - 1][j - 1]
			+ board[i - 1][j]
			+ board[i - 1][j + 1]
			+ board[i][j - 1]
			+ board[i][j + 1]
			+ board[i + 1][j - 1]
			+ board[i + 1][j]
			+ board[i + 1][j + 1]);
	return (board[i][j] == 1 && sum == 2) || sum == 3;
}


void simple_board(char board[boardy][boardx], int deltaY = 0, int deltaX = 0) { // predefined pattern for testing purpose
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

void random_board() { // generate random board
	int x, y;
	for (int i = 0; i < boardx_actual * boardy_actual / 10; i++) {
		x = rand() % boardx_actual + 1;
		y = rand() % boardy_actual + 1;
		board[y][x] = 1;
	}
}

void run_with_display() {
	memset(board, 0, board_mem_size);
	memset(temp, 0, board_mem_size);
	simple_board(board);
	simple_board(board, 10, 30);
	simple_board(board, 40, 50);
	simple_board(board, 43, 40);
	simple_board(board, -4, 3);
	while (true) {

		for (int y = 1; y <= boardy; y++) {
			for (int x = 1; x <= boardx; x++)
				std::cout << (char)(board[y][x] + 32);
			std::cout << "\n";
		}
		std::cin.get();
		std::cout.flush();
		gotoxy(0, 0);
		
		// compute the actual generation

		//#pragma omp parallel for
		for (int y = 1; y < boardy_actual; y++)
			for (int x = 1; x < boardx_actual; x++) {
				temp[y][x] = compute(y, x);
			}

		memcpy(board, temp, board_mem_size);

		Sleep(500);

		if (GetKeyState('q') & 0x8000) {
			return;
		}
	}
}

double execute_sequencial() {
	double tim;

	initTimmer(tim);

	memset(board, 0, board_mem_size);
	memset(temp, 0, board_mem_size);
	random_board();

	for (int i = 0; i < generations; i++) {
		for (int y = 1; y <= boardy_actual; y++)
			for (int x = 1; x <= boardx_actual; x++) {
				temp[y][x] = compute(y, x);
			}

		memcpy(board, temp, board_mem_size);
	}

	computeTimmer(tim);

	return tim; // in millis
}

double execute_parallel() {
	double tim;

	initTimmer(tim);

	memset(board, 0, board_mem_size);
	memset(temp, 0, board_mem_size);
	random_board();

	for (int i = 0; i < generations; i++) {
		#pragma omp parallel for
		for (int y = 1; y <= boardy_actual; y++)
			for (int x = 1; x <= boardx_actual; x++) {
				temp[y][x] = compute(y, x);
			}

		memcpy(board, temp, board_mem_size);
	}

	computeTimmer(tim);

	return tim; // in millis
}

int main() {
	memset(board, 0, board_mem_size);
	memset(temp, 0, board_mem_size);

#if use_display == true

	run_with_display();

#else

	std::cout << "time for sequencial is " << execute_sequencial() << std::endl;

	omp_set_num_threads(16);
	std::cout << "time for parallel 16 threads (max logical cores) is  " << execute_parallel() << std::endl;
	omp_set_num_threads(8);
	std::cout << "time for parallel 8 threads (max physiscal cores) is " << execute_parallel() << std::endl;

	omp_set_num_threads(6);
	std::cout << "time for parallel 6 threads is " << execute_parallel() << std::endl;
	omp_set_num_threads(5);
	std::cout << "time for parallel 5 threads is " << execute_parallel() << std::endl;
	omp_set_num_threads(4);
	std::cout << "time for parallel 4 threads is " << execute_parallel() << std::endl;
	omp_set_num_threads(2);
	std::cout << "time for parallel 2 threads is " << execute_parallel() << std::endl;

#endif


	return 0;
}