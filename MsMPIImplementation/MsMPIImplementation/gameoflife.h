#pragma once


extern void init_gof(int x, int y, int no_proc, int procID); // all processes must init

extern void master_proc_init(char* boardt, int sX, int sY); // call in master process (process id 0) only

extern void proc_init(); // call in all processes, but master (id not 0)

extern void step(bool displayOn); //call with false in master, and use use_display in others

extern void get_display_board(char* boardtd); //call in master if you want to display

extern void retDisplay();