#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> //strdup
#include <pthread.h>
#include <errno.h>
#include <unistd.h> //opts

#define VERSION 0.1
#define THRESHOLDTEMP 80
#define ADJTHRESHOLDERROR 5
#define RANDSEED 89762123
//MPI COMM TAGS
#define BASENODEHBEAT 0
#define NODENODEHBEAT 1
#define BASENODESHUTDOWN 2
#define NODENODETEMPREQ 3
#define NODENODETEMPREPLY 4
#define NODEBASEREPORT 5
#define NOCHARSTOREAD 1

// OR RUN
// mpicc main.c -o Out -lm -lpthread
// mpirun -oversubscribe -np 21 Out -n 4 -m 5 -o output.txt -i 20 -v 1



int getRow(int input_rank, int m_cols) {
        return input_rank / m_cols;
}

int getCol(int input_rank, int m_cols)
{
        return input_rank % m_cols;
}

int getRank(int input_row, int input_col, int n_rows, int m_cols)
{
        if (input_col < 0 || input_col >= m_cols)
        {
        return -1;
        }
        else if (input_row < 0 || input_row >= n_rows)
        {
        return -1;
        }
        return input_row * m_cols + input_col;
}

int *getAdjRanks(int input_rank, int n_rows, int m_cols)
{
        int row = getRow(input_rank, m_cols);
        int col = getCol(input_rank, m_cols);
        //a[0]=Up, a[1]=Down, a[2]=Left, a[3]=Right
        int *adjRanks = malloc(sizeof(int)*4);
        adjRanks[0] = getRank(row, col - 1, n_rows, m_cols);
        adjRanks[1] = getRank(row, col + 1, n_rows, m_cols);
        adjRanks[2] = getRank(row - 1, col, n_rows, m_cols);
        adjRanks[3] = getRank(row + 1, col, n_rows, m_cols);
        return adjRanks;
}

int randomRange(int min, int max)
{
        // return min + rand() / (RAND_MAX / (max - min + 1) + 1);
        return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

int randomTemp()
{
        // temperature readings range from 50 to 95
        // return rand() % (95 + 1 - 50) + 50;
        return randomRange(50, 95);
}

double dblRandomTemp() {
        return (double) randomRange(5000,9500) / 100;
}

int randomID(int n_rows, int m_cols)
{
        // returns a random grid ID which can be converted into a grid co-ord
        return randomRange(0, n_rows * m_cols);
}

int randomNode(int wsn_grid_size)
{
        // returns a random grid ID which can be converted into a grid co-ord
        return randomRange(0, wsn_grid_size-1);
}

char *randomMAC()
{
        //Creates and returns an array of len 6 with ints ranging from 0 to 255
        //Ints can be converted to hexadecimal representation for mac address values
        FILE *f;
        f = fopen("/dev/urandom","r");
        int i;
        unsigned char* randMac = malloc(sizeof(char)*6);
        for (i = 0; i < 6; i++) fread(&randMac[i], sizeof(char),1, f);

        return randMac;
}


char *randomIP()
{
        FILE *f;
        f = fopen("/dev/urandom","r");
        int i;
        unsigned char* randIP = malloc(sizeof(char)*4);
        for (i = 0; i < 6; i++) fread(&randIP[i], sizeof(char),1, f);
        return randIP;
}


/**
 * Microsleep function from stackoverflow
 * https://stackoverflow.com/a/1157217
 * */
int microsleep(long msec) 
{
        struct timespec ts;
        int res;

        if(msec < 0) {
                errno = EINVAL;
                return -1;
        }
        ts.tv_sec = msec /10000;
        ts.tv_nsec = (msec % 1000) * 1000000;
        
        do {
                res = nanosleep(&ts, &ts);
        } while (res && errno == EINTR);

    return res;

}


void show_usage(char *argvZero)
{
        // version
        printf("%s v%4.2f\n", argvZero, VERSION);

        // syntax
        printf("Usage: ");
        printf("%s", argvZero);
        printf(" -n <num of rows>");
        printf(" -m <num of cols>");
        printf(" -i <num of loop iterations");
        printf(" -o |OPTIONAL| <log output file>");
        printf(" [-h]");
        printf("\n");
}


void safeExit() {
        MPI_Finalize();
        exit(1);
}

void barrier_performance_measure(MPI_Comm comm, char * message) {
        struct timespec start_clock_time; 
        struct timespec end_clock_time; 
        double elapsed_time; 


        clock_gettime(CLOCK_MONOTONIC, &start_clock_time);
        MPI_Barrier(comm);
        clock_gettime(CLOCK_MONOTONIC, &end_clock_time);

        elapsed_time = (end_clock_time.tv_sec - start_clock_time.tv_sec) * 1e9;
        elapsed_time = (elapsed_time + (end_clock_time.tv_nsec - start_clock_time.tv_nsec)) * 1e-9;

        FILE *fbar = fopen("barrier_perf.txt","a");
        
                fprintf(fbar, "---------------------------------------------------------------------------------------------------------------------------------------\n");
                fprintf(fbar, "MPI Barrier for %s \n", message);
                fprintf(fbar, "Barrier took %lf time\n", elapsed_time);
        fclose(fbar);

}


int main(int argc, char *argv[])
{
        srand(RANDSEED);
        int opt;
        int flagn = 0, flagm = 0, flagh = 0, flago = 0, flagi=0, flagv = 0;
        int n_rows = 2, m_cols =2, i_iterations = 50, v_interval = 10;
        sat_temp_threshold = 10;
        int p_error;
        // char *outputFile;
        // outputFile = "logfile.txt";


       if (argc < 2)
        {
               show_usage(argv[0]);
               return (-1);
        }

        while ((opt = getopt(argc, argv, ":n:m:i:v:h")) != -1)
        {
               switch (opt)
               {
               case 'n':
                       flagn = 1;
                       n_rows = atoi(optarg);
                       break;
               case 'm':
                       flagm = 1;
                       m_cols = atoi(optarg);
                       break;

               case 'i':
                       flagi = 1;
                       i_iterations = atoi(optarg);
                       break;
               case 'v':
                       flagv = 1;
                       v_interval = atoi(optarg);
                       break;
               case ':':
                       printf("Error: option needs a value\n");
                       break;
               case 'h':
                       flagh = 1;
                       break;
               case '?':
                       flagh = 1;
                       printf("Error: unknown option: %c\n\n", optopt);
                       return (-1);
               }
        }

        if (flagh)
        { //help
                show_usage(argv[0]);
                return (0);
        }
        else
        { //incomplete args
                if (!(flagn && flagm && flagi && flagv))
                {
                printf("Error: required parameters are missing \n\n");
                show_usage(argv[0]);
                return (-1);
                }
        }


        FILE *fbar = fopen("barrier_perf.txt","w");
        fclose(fbar);
        
        printf("here");
        fflush(stdout);

        int wsn_size = n_rows*m_cols;


        int rank, size, rc;

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        // separating sensor nodes from main node
        MPI_Comm childComm;
        MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &childComm); 

        // Initialising struct packet sent between sensor nodes
        struct NeighbourPacket
        {
                int nodeRank;
                int nodeTemp;
                unsigned char nodeIP[4];
                unsigned char nodeMAC[6];
        };

        
        MPI_Datatype NeighbourPacketType;
        int NeighbourPacketLengths[4] = {1,1,4,6};
        const MPI_Aint NeighbourPacketDisplacements[4] = { 0, sizeof(int), 2*sizeof(int),2*sizeof(int)+4*sizeof(char)};
        MPI_Datatype NeighbourPacketTypes[4] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR};
        MPI_Type_create_struct(4, NeighbourPacketLengths, NeighbourPacketDisplacements, NeighbourPacketTypes, &NeighbourPacketType);
        MPI_Type_commit(&NeighbourPacketType);
        
        // creating struct to be sent when temperature exceeds threshold
        struct MainSendReport
        {
                int nodeRank;
                int nodesCompared;
                unsigned char nodeIP[4];
                unsigned char nodeMAC[6];
                unsigned char adjNodesIP[16];
                unsigned char adjNodesMAC[24];
                int nodeTemp;
                int adjNodesTemp[4];
                double reportTime;
        }; 

        struct MainSendReport offsetCalcReportStruct;
        MPI_Aint MainSendReportAdresses[9];
        MPI_Aint offsets[9];
        MPI_Get_address(&offsetCalcReportStruct, &MainSendReportAdresses[0]);
        MPI_Get_address(&offsetCalcReportStruct.nodesCompared, &MainSendReportAdresses[1]);
        MPI_Get_address(&offsetCalcReportStruct.nodeIP, &MainSendReportAdresses[2]);
        MPI_Get_address(&offsetCalcReportStruct.nodeMAC, &MainSendReportAdresses[3]);
        MPI_Get_address(&offsetCalcReportStruct.adjNodesIP, &MainSendReportAdresses[4]);
        MPI_Get_address(&offsetCalcReportStruct.adjNodesMAC, &MainSendReportAdresses[5]);
        MPI_Get_address(&offsetCalcReportStruct.nodeTemp, &MainSendReportAdresses[6]);
        MPI_Get_address(&offsetCalcReportStruct.adjNodesTemp, &MainSendReportAdresses[7]);
        MPI_Get_address(&offsetCalcReportStruct.reportTime, &MainSendReportAdresses[8]);
        offsets[0] = 0;
        for(int i = 1; i < 9; i++) offsets[i] = MainSendReportAdresses[i] - MainSendReportAdresses[0];
        
        int blocklengths[9] = {1, 1, 4, 6, 16, 24, 1, 4, 1};
        MPI_Datatype types[9] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_DOUBLE};
        MPI_Datatype MainSendReportType;
  
        MPI_Type_create_struct(9, blocklengths, offsets, types, &MainSendReportType);
        MPI_Type_commit(&MainSendReportType);   


        int cartesian_dimensions[2];
        cartesian_dimensions[0] = n_rows; /* number of rows */
        cartesian_dimensions[1] = m_cols; /* number of columns */
        // End program if user doesn't enter perfect cartesian grid (n*m + 1) values 
        if ((n_rows * m_cols) != size - 1)
        {
                if (rank == 0)
                        printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", n_rows, m_cols, n_rows * m_cols, size);
                        fflush(stdout);
                        MPI_Abort(MPI_COMM_WORLD, 1);
                        return 0;
        }

        int master_node(int rank, int v_interval, int i_iterations, int n_rows, int m_cols, MPI_Comm main_comm, MPI_Comm childComm);
        int sensor_node(int rank, int v_interval, int i_iterations, int n_rows, int m_cols, MPI_Comm main_comm, MPI_Comm childComm);

        /////////////////////////////////////////////////////// Base Station ///////////////////////////////////////////////////////////////
        if (rank == size - 1)
        {       
               master_node(int rank, int v_interval, int i_iterations, int n_rows, int m_cols, MPI_Comm main_comm, MPI_Comm childComm);

        }
        //////////////////////////////////////////////////////////////// Sensor Node ///////////////////////////////////////////////////////////////////////

        //NB - MPI MESSAGE TAGS:
        //     DummyAdjRequest  Tag:0
        //     AdjNodeTempReq   Tag:1
        //     AdjNodeTempReply Tag:2
        // 
        else
        {
                sensor_node(rank, v_interval, i_iterations, n_rows, m_cols, main_comm, childComm);
        }
        MPI_Finalize();                                         
        return 0;
}
        
      


