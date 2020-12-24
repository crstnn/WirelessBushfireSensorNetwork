#ifndef UTILS_H
#define UTILS_H

#include "simulator.h"

int getRow(int input_rank, int m_cols);
int getCol(int input_rank, int m_cols);
int getRank(int input_row, int input_col, int n_rows, int m_cols);
int *getAdjRanks(int input_rank, int n_rows, int m_cols);
int randomRange(int min, int max);
int randomTemp();
double dblRandomTemp();
int randomID(int n_rows, int m_cols);
int randomNode(int wsn_grid_size);
char *randomMAC();
char *randomIP();
void barrier_performance_measure(MPI_Comm comm, char *message);
void show_usage(char *argvZero);
void setSeed(unsigned int rank);
int file_exists(char *filename);

#endif