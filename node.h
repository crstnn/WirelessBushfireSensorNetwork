#ifndef NODE_H
#define NODE_H

#include "simulator.h"
#include "utils.h"

int sensor_node(int rank, int v_interval, int i_iterations, int n_rows, int m_cols,
                MPI_Datatype neighbour_packet_type, MPI_Datatype main_send_report_type, MPI_Comm childComm);

#endif