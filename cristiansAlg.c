#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main(int argc, char *argv[]) {

	int rank, size, rc;
	
	MPI_Request req, req2;
    MPI_Status status;

    
    rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS) {
		printf("Error Starting MPI. Terminating\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}
    
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	double cli_time, master_time, local_time, s_time;
    
	// base station server
	if (rank == 0){
        printf("rank %d\n", rank);
		master_time = MPI_Wtime();
        printf("rank %d - beofre for\n", rank);
		for(int idx=1; idx<size; idx++) {
            printf("rank %d - in for %d \n", rank, idx);
            MPI_Isend(&master_time, 1, MPI_DOUBLE, idx, 123, MPI_COMM_WORLD, &req);
//             MPI_Wait(&req, &status);
		}
		printf("rank %d - after for\n", rank);
	}
	
	else{
        printf("rank %d\n", rank);
		s_time = MPI_Wtime();
		MPI_Irecv(&cli_time, 1, MPI_DOUBLE, 0, 1234, MPI_COMM_WORLD, &req2);
		printf("rank %d - create req\n", rank);
// 		MPI_Wait(&req, &status);
// 		MPI_Wait(&req2, &status);
		printf("rank %d - after wait\n", rank);
        // Cristian's algorithm formula
		local_time = cli_time + (MPI_Wtime() - s_time)/2;
		
		fflush(stdout);
		printf("Process %d has local time %f", rank, local_time);
	}
	
	
    MPI_Finalize();
    return 0;
}
