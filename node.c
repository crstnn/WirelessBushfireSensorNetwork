#include "node.h"

/***
 * WSN Functionality
 *      Simulates measuring a forest temperature every interval for given iterations
 *      Sends HB to Base Station for fault detection, then if overheating requests
 *      temperatures for neighbouring nodes to check for concensus. If achieved, then
 *      prepares and sends a main send report to the base station for logging.
 * */
int sensor_node(int rank, int v_interval, int i_iterations, int n_rows, int m_cols, MPI_Datatype neighbour_packet_type, MPI_Datatype main_send_report_type, MPI_Comm childComm)
{

        setSeed((unsigned)(rank) + (unsigned)time(NULL));

        // setting up some items that remain constant over time for each node
        unsigned char *node_MAC = randomMAC();
        unsigned char *node_IP = randomIP();
        int adjNodesAgree = 0, *adjNodeRanks = getAdjRanks(rank, n_rows, m_cols);
        int shutdown = 0;
        int dummyReq = 1, adjreq = 0;

        fflush(stdout);
        printf("WSN: RANK %d,\n MAC is %X:%X:%X:%X:%X:%X\n IP is %u.%u.%u.%u\n", rank,
               node_MAC[0], node_MAC[1], node_MAC[2], node_MAC[3], node_MAC[4], node_MAC[5],
               node_IP[0], node_IP[1], node_IP[2], node_IP[3]);

        // counts interval timing
        time_t startTimeSeconds;
        startTimeSeconds = time(NULL);
        time_t endTimeSeconds = time(NULL) + (v_interval * i_iterations);

        char filePath[81];
        snprintf(filePath, sizeof filePath, "./exit%d", rank);
        printf("WSN is listening for shutdown message from '%s' file\n", filePath);
        fflush(stdout);

        int flag_run_wsn = 0;
        // while (time (NULL) < endTimeSeconds)
        while (i_iterations > 0)
        {
                if ((time(NULL) - startTimeSeconds) % v_interval == 0)
                        flag_run_wsn = 1;
                sleep(1);

                if (flag_run_wsn)
                {

                        printf("\n\n-------------------------------------------------------------------------------------------------\n Iterations: %d", i_iterations);
                        fflush(stdout);
                        int node_local_rec_temp = randomTemp();

                        printf("WSN: RANK %d, TEMP: %d\n", rank, node_local_rec_temp);
                        fflush(stdout);

                        MPI_Request nodeAliveReq;
                        MPI_Status nodeAliveStat;

                        if (access(filePath, F_OK) != -1)
                        {
                                printf("Node %d : Simulating Failure\n", rank);
                                fflush(stdout);
                        }
                        else
                        {
                                MPI_Isend(&dummyReq, 1, MPI_INT, n_rows * m_cols, BASENODEHBEAT, MPI_COMM_WORLD, &nodeAliveReq);
                        }

                        // communicating with base station
                        MPI_Request nodeShutdownReq;
                        MPI_Status nodeShutdownStat;

                        MPI_Barrier(MPI_COMM_WORLD);

                        int recFlag = 0;
                        MPI_Barrier(MPI_COMM_WORLD);

                        MPI_Iprobe(n_rows * m_cols, BASENODESHUTDOWN, MPI_COMM_WORLD, &recFlag, MPI_STATUS_IGNORE);

                        // node told to shutdown if master node dictates so

                        if (recFlag == 1)
                        {

                                MPI_Recv(&shutdown, 1, MPI_INT, n_rows * m_cols, BASENODESHUTDOWN, MPI_COMM_WORLD, &nodeShutdownStat);
                        }

                        if (shutdown == 1)
                        {
                                fflush(stdout);
                                printf("%d has gracefully shutdown (after being told so by the master node)\n", rank);
                                MPI_Finalize();
                                exit(0);
                        }

                        //Initialise neightbour packet struct
                        struct NeighbourPacket NeighbourPacketMessage;

                        NeighbourPacketMessage.nodeRank = rank;
                        for (int i = 0; i < 4; i++)
                                NeighbourPacketMessage.nodeIP[i] = node_IP[i];
                        for (int i = 0; i < 6; i++)
                                NeighbourPacketMessage.nodeMAC[i] = node_MAC[i];
                        NeighbourPacketMessage.nodeTemp = node_local_rec_temp;

                        // has the current node overheated
                        int overheated = 0;
                        if (node_local_rec_temp > THRESHOLDTEMP)
                        {
                                overheated = 1;
                                printf("WSN: RANK %d, TEMP: %d has OVERHEATED!!!!!!!!!!!!\n", rank, node_local_rec_temp);
                                fflush(stdout);
                        }
                        if (overheated)
                        {
                                MPI_Request adjNodesReq;
                                MPI_Status adjNodesStat;

                                // check vertical and horizonal nodes (within (less than) 5 degrees of the request node) because current sensor has overheated
                                printf("WSN: RANK %d, TEMP: %d is SENDING ADJ REQS\n", rank, node_local_rec_temp);
                                fflush(stdout);
                                for (int i = 0; i < 4; i++)
                                {
                                        if (adjNodeRanks[i] != -1)
                                        {
                                                int sendMessage = 1;
                                                MPI_Isend(&sendMessage, 1, MPI_INT, adjNodeRanks[i], NODENODETEMPREQ, MPI_COMM_WORLD, &adjNodesReq);
                                        }
                                }
                        }

                        printf("WSN: RANK %d, TEMP: %d is waiting for sends to complete\n", rank, node_local_rec_temp);
                        fflush(stdout);

                        if (rank == 0)
                        {
                                char *bar_msg1 = "WSN OVERHEAT -Req Sync BARRIER ";
                                barrier_performance_measure(childComm, bar_msg1);
                        }
                        else
                        {
                                MPI_Barrier(childComm);
                        }

                        // checking whether current node who has requested from adjacent nodes receives a response message
                        printf("WSN: RANK %d, is checking for temp reqs from adj nodes\n", rank);
                        fflush(stdout);
                        MPI_Request NeighbourPacketSendRequest;
                        int receivedFlag[] = {0, 0, 0, 0};
                        for (int i = 0; i < 4; i++)
                        {
                                if (adjNodeRanks[i] != -1)
                                        MPI_Iprobe(adjNodeRanks[i], NODENODETEMPREQ, MPI_COMM_WORLD, &receivedFlag[i], MPI_STATUS_IGNORE);
                        }

                        printf("WSN: RANK %d, TEMP: %d\n is accepting adj node reqs and sending its data\n", rank, node_local_rec_temp);
                        fflush(stdout);
                        for (int i = 0; i < 4; i++)
                        {
                                int recMessage = 0;
                                if (receivedFlag[i])
                                {
                                        MPI_Recv(&recMessage, 1, MPI_INT, adjNodeRanks[i], NODENODETEMPREQ, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                                }
                                if (recMessage == 1)
                                {
                                        //send packet to requesting neighbour
                                        MPI_Isend(&NeighbourPacketMessage, 1, neighbour_packet_type, adjNodeRanks[i], NODENODETEMPREPLY, MPI_COMM_WORLD, &NeighbourPacketSendRequest);
                                }
                        }

                        if (rank == 0)
                        {
                                char *bar_msg2 = "WSN OVERHEAT  - Recieve Response Sync BARRIER ";
                                barrier_performance_measure(childComm, bar_msg2);
                        }
                        else
                        {
                                MPI_Barrier(childComm);
                        }

                        if (overheated)
                        {
                                struct NeighbourPacket neighbourPacketReplies[4];
                                // receiving temps from adj nodes

                                int adjNodeTemps[4];
                                for (int i = 0; i < 4; i++)
                                {
                                        if (adjNodeRanks[i] != -1)
                                        {
                                                printf("WSN: RANK %d, TEMP: %d\n is checking local reports from adj nodes\n", rank, node_local_rec_temp);
                                                fflush(stdout);
                                                MPI_Request NeighbourPacketRecvRequest;
                                                MPI_Irecv(&neighbourPacketReplies[i], 1, neighbour_packet_type, adjNodeRanks[i], NODENODETEMPREPLY, MPI_COMM_WORLD, &NeighbourPacketRecvRequest);
                                                adjNodeTemps[i] = neighbourPacketReplies[i].nodeTemp;

                                                printf("WSN: RANK %d received report from adj node rank: %d\n", rank, neighbourPacketReplies[i].nodeRank);
                                                printf("adj node temp: %d , node %u.%u.%u.%u node mac %X:%X:%X:%X:%X:%X\n", neighbourPacketReplies[i].nodeTemp,
                                                       neighbourPacketReplies[i].nodeIP[0], neighbourPacketReplies[i].nodeIP[1], neighbourPacketReplies[i].nodeIP[2], neighbourPacketReplies[i].nodeIP[3],
                                                       neighbourPacketReplies[i].nodeMAC[0], neighbourPacketReplies[i].nodeMAC[1], neighbourPacketReplies[i].nodeMAC[2], neighbourPacketReplies[i].nodeMAC[3], neighbourPacketReplies[i].nodeMAC[4], neighbourPacketReplies[i].nodeMAC[5]);
                                        }
                                        else
                                        {
                                                adjNodeTemps[i] = -1;
                                        }
                                }
                                printf("adjacent node ranks: %d %d %d %d\n", adjNodeRanks[0], adjNodeRanks[1], adjNodeRanks[2], adjNodeRanks[3]);

                                // Design note: adjacent nodes can be THRESHOLDTEMP - ADJTHRESHOLDERROR in order for error to alert
                                int validTempConsensusCount = 0;
                                for (int i = 0; i < sizeof(adjNodeTemps) / sizeof(adjNodeTemps[0]); i++)
                                {
                                        if (adjNodeTemps[i] > THRESHOLDTEMP - ADJTHRESHOLDERROR)
                                                validTempConsensusCount++;
                                }

                                if (validTempConsensusCount >= 2)
                                {

                                        printf("RANK %d has reached a local consensus of >= 1 nodes overheating\n", rank);
                                        fflush(stdout);

                                        // amalgamating adj nodes and current node into one send packet (struct) to to send back to base station
                                        struct MainSendReport MainSendReportMessage;
                                        MainSendReportMessage.nodeRank = rank;
                                        MainSendReportMessage.nodesCompared = validTempConsensusCount;
                                        for (int i = 0; i < 6; i++)
                                                MainSendReportMessage.nodeMAC[i] = node_MAC[i];
                                        for (int i = 0; i < 4; i++)
                                                MainSendReportMessage.nodeIP[i] = node_IP[i];
                                        for (int i = 0; i < 4; i++)
                                                for (int j = 0; j < 6; j++)
                                                        MainSendReportMessage.adjNodesMAC[i * 6 + j] = neighbourPacketReplies[i].nodeMAC[j];
                                        for (int i = 0; i < 4; i++)
                                                for (int j = 0; j < 4; j++)
                                                        MainSendReportMessage.adjNodesIP[i * 4 + j] = neighbourPacketReplies[i].nodeIP[j];
                                        MainSendReportMessage.nodeTemp = node_local_rec_temp;
                                        for (int i = 0; i < 4; i++)
                                                MainSendReportMessage.adjNodesTemp[i] = adjNodeTemps[i];
                                        MainSendReportMessage.reportTime = MPI_Wtime();

                                        MPI_Request reportMsgReq;
                                        MPI_Status reportMsgStat;

                                        MPI_Isend(&MainSendReportMessage, 1, main_send_report_type, n_rows * m_cols, NODEBASEREPORT, MPI_COMM_WORLD, &reportMsgReq);
                                }
                        }
                        MPI_Barrier(MPI_COMM_WORLD);
                        i_iterations--;
                }
                flag_run_wsn = 0;
        }
}