
#include "utils.h"

/**
 * Function to determine row co-ord of a node from its rank and grid size
 **/
int getRow(int input_rank, int m_cols)
{
        return input_rank / m_cols;
}

/**
 * Function to determine col co-ord of a node from its rank and grid size
 **/
int getCol(int input_rank, int m_cols)
{
        return input_rank % m_cols;
}

/**
 * Function to determine rank of a node from its co-ord
 **/
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

/**
 * Function to determine ranks of adjnodes from its rank and grid size
 **/
int *getAdjRanks(int input_rank, int n_rows, int m_cols)
{
        int row = getRow(input_rank, m_cols);
        int col = getCol(input_rank, m_cols);
        //a[0]=Up, a[1]=Down, a[2]=Left, a[3]=Right
        int *adjRanks = malloc(sizeof(int) * 4);
        adjRanks[0] = getRank(row, col - 1, n_rows, m_cols);
        adjRanks[1] = getRank(row, col + 1, n_rows, m_cols);
        adjRanks[2] = getRank(row - 1, col, n_rows, m_cols);
        adjRanks[3] = getRank(row + 1, col, n_rows, m_cols);
        return adjRanks;
}

/**
 * Function to set seed of random number generator
 *   used by each MPI process to get a distinct rng 
 * */
void setSeed(unsigned int rank)
{
        srand(rank);
}

/**
 * Function to get a random number within an given inclusive range
 **/
int randomRange(int min, int max)
{
        // return min + rand() / (RAND_MAX / (max - min + 1) + 1);
        return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

/**
 * Function to generate a random integer temperature
 **/
int randomTemp()
{
        // temperature readings range from 50 to 95
        // return rand() % (95 + 1 - 50) + 50;
        return randomRange(50, 95);
}

/**
 * Function to generate a random double temperature
 **/
double dblRandomTemp()
{
        return (double)randomRange(5000, 9500) / 100;
}

/**
 * Function to generate a random node from the row and col size
 *      used by the sat thread to choose a random sensor to record temp for
 **/
int randomID(int n_rows, int m_cols)
{
        // returns a random grid ID which can be converted into a grid co-ord
        return randomRange(0, n_rows * m_cols);
}

/**
 *  Function to generate a random node from the grid size
 *      used by the sat thread to choose a random sensor to record temp for
 **/
int randomNode(int wsn_grid_size)
{
        // returns a random grid ID which can be converted into a grid co-ord
        return randomRange(0, wsn_grid_size - 1);
}

/**
 *  Function to generate a random Mac address
 *     uses unsigned chars to store each 8 bit section of the mac
 **/
char *randomMAC()
{
        //Creates and returns an array of len 6 with ints ranging from 0 to 255
        //Ints can be converted to hexadecimal representation for mac address values
        FILE *f;
        f = fopen("/dev/urandom", "r");
        int i;
        unsigned char *randMac = malloc(sizeof(char) * 6);
        for (i = 0; i < 6; i++)
                fread(&randMac[i], sizeof(char), 1, f);

        return randMac;
}

/**
 *  Function to generate a random ip address
 *     uses unsigned chars to store each 8 bit section of the ip
 **/
char *randomIP()
{
        FILE *f;
        f = fopen("/dev/urandom", "r");
        int i;
        unsigned char *randIP = malloc(sizeof(char) * 4);
        for (i = 0; i < 6; i++)
                fread(&randIP[i], sizeof(char), 1, f);
        return randIP;
}

/**
 *  Function to make mpi barrier calls and log the time spent at each instance
 **/
void barrier_performance_measure(MPI_Comm comm, char *message)
{
        struct timespec start_clock_time;
        struct timespec end_clock_time;
        double elapsed_time;

        clock_gettime(CLOCK_MONOTONIC, &start_clock_time);
        MPI_Barrier(comm);
        clock_gettime(CLOCK_MONOTONIC, &end_clock_time);

        elapsed_time = (end_clock_time.tv_sec - start_clock_time.tv_sec) * 1e9;
        elapsed_time = (elapsed_time + (end_clock_time.tv_nsec - start_clock_time.tv_nsec)) * 1e-9;

        int comm_cmp;
        MPI_Comm_compare(MPI_COMM_WORLD, comm, &comm_cmp);
        if (comm_cmp == MPI_IDENT)
        {
                FILE *fbar = fopen("barrier_perf_world.txt", "a");

                fprintf(fbar, "---------------------------------------------------------------------------------------------------------------------------------------\n");
                fprintf(fbar, "MPI Barrier for %s \n", message);
                fprintf(fbar, "Barrier took %lf time\n", elapsed_time);
                fclose(fbar);
        }
        else
        {
                FILE *fbar = fopen("barrier_perf_child.txt", "a");
                fprintf(fbar, "---------------------------------------------------------------------------------------------------------------------------------------\n");
                fprintf(fbar, "MPI Barrier for %s \n", message);
                fprintf(fbar, "Barrier took %lf time\n", elapsed_time);
                fclose(fbar);
        }
}

/**
 *  Function to show cmd line arguments for the simulator.h program
 **/
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

/**
 *  Function to check if a file exists
 *      using stat
 **/
int file_exists(char *filename)
{
        struct stat buffer;
        return (stat(filename, &buffer) == 0);
}
