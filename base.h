#ifndef BASE_H
#define BASE_H

#include "simulator.h"
#include "utils.h"

struct sat_thr_arg_struct
{
        int iterations;
        int interval;
        int grid_size;
};

int base_station(int rank, int v_interval, int i_iterations, int n_rows, int m_cols,
                 MPI_Datatype main_send_report_type, MPI_Comm childComm);
#endif