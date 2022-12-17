#include <iostream>
#include <windows.h>
#include <omp.h>

#define use_display true

#if use_display == true

#define boardx_actual 80
#define boardy_actual 40

#else

#define boardx_actual 80
#define boardy_actual 40

#endif

#define bordx (boardx_actual + 2)
#define bordy (boardy_actual + 2)

char temp[bordy + 2][bordx + 2], board[bordy + 2][bordx + 2];

size_t board_mem_size = bordx * bordy * sizeof(temp[0][0]);

#define generations 100000

#define initTimmer(t) t = omp_get_wtime()
#define computeTimmer(t) t = omp_get_wtime() - t

inline void gotoxy(short col, short line) {
	SetConsoleCursorPosition(
		GetStdHandle(STD_OUTPUT_HANDLE),
		COORD{ col, line }
	);
}

char compute(int i, int j) {
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

void compute_generation() {
	for (int y = 1; y <= bordy; y++)
		for (int x = 1; x <= bordx; x++) {
			temp[y][x] = compute(y, x);
		}

	memcpy(board, temp, board_mem_size);
}

void compute_generation_parralel_omp() {
	#pragma omp parallel for
	for (int y = 1; y <= bordy; y++)
		for (int x = 1; x <= bordx; x++) {
			temp[y][x] = compute(y, x);
		}

	memcpy(board, temp, board_mem_size);
}

void simple_board() {
	board[11][10] = board[11][11] = board[11][12] = 1;
	board[12][11] = 1;
	board[13][11] = 1;
	board[14][10] = board[14][11] = board[14][12] = 1;

	board[16][10] = board[16][11] = board[16][12] = 1;
	board[17][10] = board[17][11] = board[17][12] = 1;

	board[19][10] = board[19][11] = board[19][12] = 1;
	board[20][11] = 1;
	board[21][11] = 1;
	board[22][10] = board[22][11] = board[22][12] = 1;


	board[11][41] = board[11][42] = board[12][41] = 1;
	board[14][43] = board[14][44] = board[13][44] = 1;

	board[11][30] = board[11][31] = board[11][32] = board[11][33] = 1;
}

void run_with_display() {
	simple_board();
	while (true) {

		for (int y = 1; y <= bordy; y++) {
			for (int x = 1; x <= bordx; x++)
				std::cout << (char)(board[y][x] + 32);
			std::cout << "\n";
		}
		std::cin.get();
		std::cout.flush();
		gotoxy(0, 0);
		compute_generation();
		Sleep(500);

		if (GetKeyState('q') & 0x8000) {
			return;
		}
	}
}

double execute_sequencial() {
	double tim;
	simple_board();

	initTimmer(tim);

	for (int i = 0; i < generations; i++) {
		compute_generation();
	}

	computeTimmer(tim);

	return tim;
}

double execute_parallel() {
	double tim;
	simple_board();

	initTimmer(tim);

	for (int i = 0; i < generations; i++) {
		compute_generation_parralel_omp();
	}

	computeTimmer(tim);

	return tim;
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
	omp_set_num_threads(4);
	std::cout << "time for parallel 4 threads is " << execute_parallel() << std::endl;
	omp_set_num_threads(2);
	std::cout << "time for parallel 2 threads is " << execute_parallel() << std::endl;

#endif


	return 0;
}