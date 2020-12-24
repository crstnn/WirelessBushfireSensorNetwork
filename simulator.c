#include "simulator.h"
#include "base.h"
#include "node.h"
#include "utils.h"

int main(int argc, char *argv[])
{
        int opt;
        int flagn = 0, flagm = 0, flagh = 0, flago = 0, flagi = 0, flagv = 0;
        int n_rows = 2, m_cols = 2, i_iterations = 50, v_interval = 10;

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

        int wsn_size = n_rows * m_cols;

        int rank, size, rc;

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        // separating sensor nodes from main node
        MPI_Comm childComm;
        MPI_Comm_split(MPI_COMM_WORLD, rank == size - 1, 0, &childComm);

        MPI_Datatype NeighbourPacketType;
        int NeighbourPacketLengths[4] = {1, 1, 4, 6};
        const MPI_Aint NeighbourPacketDisplacements[4] = {0, sizeof(int), 2 * sizeof(int), 2 * sizeof(int) + 4 * sizeof(char)};
        MPI_Datatype NeighbourPacketTypes[4] = {MPI_INT, MPI_INT, MPI_CHAR, MPI_CHAR};
        MPI_Type_create_struct(4, NeighbourPacketLengths, NeighbourPacketDisplacements, NeighbourPacketTypes, &NeighbourPacketType);
        MPI_Type_commit(&NeighbourPacketType);

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
        for (int i = 1; i < 9; i++)
                offsets[i] = MainSendReportAdresses[i] - MainSendReportAdresses[0];

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

        /////////////////////////////////////////////////////// Base Station ///////////////////////////////////////////////////////////////
        if (rank == size - 1)
        {
                base_station(rank, v_interval, i_iterations, n_rows, m_cols, MainSendReportType, childComm);
        }
        //////////////////////////////////////////////////////////////// Sensor Node ///////////////////////////////////////////////////////////////////////
        else
        {
                sensor_node(rank, v_interval, i_iterations, n_rows, m_cols, NeighbourPacketType, MainSendReportType, childComm);
        }
        MPI_Finalize();
        return 0;
}