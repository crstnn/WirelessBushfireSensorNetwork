#include "base.h"

pthread_mutex_t satellite_Mutex = PTHREAD_MUTEX_INITIALIZER;

int curr_satellite_temp;
int curr_satellite_time;
int sat_wsn_time_flag;
int sat_wsn_temp_flag;
int satellite_shutdown;
double **satellite_reports;

void *satelliteThreadFunc(void *pArg)
{
        setSeed((unsigned)time(NULL));
        printf("Sat Pthread created\n");
        struct sat_thr_arg_struct *args = pArg;

        time_t sat_clock_time;
        time_t sat_startTimeSeconds;
        sat_startTimeSeconds = time(NULL);
        time_t sat_endTimeSeconds = time(NULL) + (args->interval * args->iterations);

        int sat_flag_run = 0;
        int sat_loop_end_flag = 0;
        printf("iter: %d, intv: %d", args->iterations, args->interval);
        while (args->iterations > 0 && !sat_loop_end_flag)
        {
                if ((time(NULL) - sat_startTimeSeconds) % args->interval == 0)
                        sat_flag_run = 1;
                sleep(1);
                if (sat_flag_run)
                {
                        printf("Sat Pthread generating values\n");
                        pthread_mutex_lock(&satellite_Mutex);
                        sat_loop_end_flag = satellite_shutdown;
                        for (int j = 0; j < args->grid_size; j++)
                        {
                                satellite_reports[j][0] = (double)randomTemp();
                                satellite_reports[j][1] = MPI_Wtime();
                                printf("SatReports: node- %d ,temp %f  \n", j, satellite_reports[j][0]);
                                printf("SatReports: node- %d ,time %f  \n", j, satellite_reports[j][1]);
                        }
                        pthread_mutex_unlock(&satellite_Mutex);
                        args->iterations -= 1;
                }
                sat_flag_run = 0;
        }
}

void *satelliteThreadFunc2(void *pArg)
{
        setSeed((unsigned)time(NULL));
        printf("Sat Pthread2 created\n");
        fflush(stdout);
        struct sat_thr_arg_struct *args = pArg;
        while (1)
        {
                pthread_mutex_lock(&satellite_Mutex);
                if (satellite_shutdown)
                        break;
                int sat_curr_coord = randomNode(args->grid_size);
                printf("sat thread curr node %d \n", sat_curr_coord);
                satellite_reports[sat_curr_coord][0] = (double)randomTemp();
                satellite_reports[sat_curr_coord][1] = MPI_Wtime();
                printf("sat thread temp %f \n", satellite_reports[sat_curr_coord][0]);
                pthread_mutex_unlock(&satellite_Mutex);
                sleep(10);
        }
        printf("Thread finished\n");
        // fflush(stdout);
        pthread_exit(NULL);
}

int base_station(int rank, int v_interval, int i_iterations, int n_rows, int m_cols, MPI_Datatype main_send_report_type, MPI_Comm childComm)
{
        //clear both report files
        fclose(fopen("logfile.txt", "w"));
        fclose(fopen("performanceMetrics.txt", "w"));
        fclose(fopen("barrier_perf_world.txt", "w"));
        fclose(fopen("barrier_perf_child.txt", "w"));

        int noOfMessages = 0;
        //Initialise Pthread, Mutex and Satellite Report Array

        satellite_reports = malloc((n_rows * m_cols) * sizeof(double *));
        for (int i = 0; i < n_rows * m_cols; i++)
        {
                satellite_reports[i] = calloc(2, sizeof(double));
        }

        struct sat_thr_arg_struct args;
        args.interval = v_interval;
        args.iterations = i_iterations;
        args.grid_size = n_rows * m_cols;

        satellite_shutdown = 0;

        printf("Starting pthead from base\n");
        pthread_t satellite_thread;
        pthread_mutex_init(&satellite_Mutex, NULL);
        int p_error = pthread_create(&satellite_thread, NULL, satelliteThreadFunc2, (void *)&args);
        if (p_error != 0)
        {
                FILE *f = fopen("logfile.txt", "w");
                printf("Error:unable to create satellite thread, %d\n", p_error);
                fprintf(f, "Base Station failed to create satellite thread\npthread_create failed with satellite thread. errno = %d, %s\n", p_error, strerror(p_error));
                fclose(f);
                exit(-1);
        };
        printf("Perror: %d \n", p_error);

        int confirmedReports = 0;
        int falseReports = 0;

        time_t startTimeSeconds;
        startTimeSeconds = time(NULL);
        time_t endTimeSeconds = time(NULL) + (v_interval * i_iterations);

        int flag_run = 0;
        // while (time (NULL) < endTimeSeconds) {
        while (i_iterations > 0)
        {

                if ((time(NULL) - startTimeSeconds) % v_interval == 0)
                        flag_run = 1;
                sleep(1);

                if (flag_run)
                {

                        printf("-------------------------------------------------------------------------------------------------\nIterations: %d\n\n", i_iterations);
                        fflush(stdout);

                        MPI_Request nodesAliveReq[n_rows * m_cols];
                        MPI_Status nodesAliveStat[n_rows * m_cols];

                        int *isNodesAliveArr = calloc(n_rows * m_cols, sizeof(int));

                        char *bar_msg1 = "Starting BASE BARRIER";
                        barrier_performance_measure(MPI_COMM_WORLD, bar_msg1);

                        printf("Master receiving whether child nodes are alive\n");
                        fflush(stdout);

                        int numberOfNodeNonResponsive = 0;

                        // checking whether the child nodes are alive (FAULT TOLERANCE)
                        int *isNodeAliveReceivedFlag = calloc(n_rows * m_cols, sizeof(int));
                        for (int i = 0; i < n_rows * m_cols; i++)
                        {
                                MPI_Iprobe(i, BASENODEHBEAT, MPI_COMM_WORLD, &isNodeAliveReceivedFlag[i], MPI_STATUS_IGNORE);
                        }
                        for (int i = 0; i < n_rows * m_cols; i++)
                        {
                                if (isNodeAliveReceivedFlag[i])
                                {
                                        MPI_Irecv(&isNodesAliveArr[i], 1, MPI_INT, i, BASENODEHBEAT, MPI_COMM_WORLD, &nodesAliveReq[i]);
                                }
                                else
                                {
                                        FILE *f2 = fopen("performanceMetrics.txt", "a");
                                        if (f2 == NULL)
                                        {
                                                printf("Error opening file!\n");
                                                exit(1);
                                        }
                                        // logging fault 
                                        if (numberOfNodeNonResponsive == 0){
                                                fprintf(f2, "---------------------------------------------NETWORK FAULT - GRACEFUL SHUTDOWN ENSUED-----------------------------------------------\n");

                                        }

                                        // Design note: we have designed this program to confirmedReports multiple fault reports at every interval

                                        time_t timer;
                                        char buff[26];
                                        struct tm *tm_info;

                                        timer = time(NULL);
                                        tm_info = localtime(&timer);

                                        strftime(buff, 26, "%Y-%m-%d %H:%M:%S", tm_info);

                                        fprintf(f2, "Sensor node %d was non-responsive and the base station became aware at time %s \n", i, buff);
                                        
                                        fclose(f2);

                                        numberOfNodeNonResponsive ++;
                                        isNodesAliveArr[i] = 0;
                                        printf("DID NOT RECIEVE HEARTBEAT FROM NODE RANK %d \n", i);
                                        fflush(stdout);
                                }
                        }

                        int terminationVal = 1;
                        char isSentinelFileShutdown = '0';

                        // reading 0 or 1 from shutdown file specified by the user
                        FILE *sF = fopen("isShutdown.txt", "r");

                        if (sF != NULL)
                        {
                                isSentinelFileShutdown = fgetc(sF);
                                fclose(sF);
                        }


                        MPI_Request sdownReq[n_rows * m_cols];

                        // if a child node has not responded we shutdown the network, gracefully (FAULT TOLERANCE) OR if user has specifed from the sentinel file to shutdown the network
                        for (int i = 0; i < n_rows * m_cols; i++)
                        {
                                if (!isNodesAliveArr[i] || isSentinelFileShutdown == '1')
                                {
                                        satellite_shutdown = 1;
                                        printf("Master sending termination to child nodes. Terminated gracefully \n");
                                        fflush(stdout);
                                        // sending network termination message
                                        for (int i = 0; i < n_rows * m_cols; i++)
                                                MPI_Isend(&terminationVal, 1, MPI_INT, i, BASENODESHUTDOWN, MPI_COMM_WORLD, &sdownReq[i]);
                                        char *bar_msg2 = "Starting BASE BARRIER";
                                        barrier_performance_measure(MPI_COMM_WORLD, bar_msg2);

                                        FILE *f2 = fopen("performanceMetrics.txt", "a");
                                        if (f2 == NULL)
                                        {
                                                printf("Error opening file!\n");
                                                exit(1);
                                        }

                                        if (isSentinelFileShutdown == '1'){
                                                fprintf(f2, "---------------------------------------------SENTINEL FILE DICTATED GRACEFUL SHUTDOWN---------------------------------------------\n");
                                        }

                                        fclose(f2);

                                        fflush(stdout);

                                        MPI_Finalize();
                                        exit(0);
                                }
                        }
                        char *bar_msg3 = "Starting BASE BARRIER";
                        barrier_performance_measure(MPI_COMM_WORLD, bar_msg3);

                        printf("Master Did not terminate nodes\n");
                        fflush(stdout);
                        char *bar_msg4 = "Starting BASE BARRIER";
                        barrier_performance_measure(MPI_COMM_WORLD, bar_msg4);

                        printf("Base about to check for node-base reports from nodes\n");
                        fflush(stdout);
                        int *receivedFlag = calloc(n_rows * m_cols, sizeof(int));
                        for (int i = 0; i < n_rows * m_cols; i++)
                        {
                                MPI_Iprobe(i, NODEBASEREPORT, MPI_COMM_WORLD, &receivedFlag[i], MPI_STATUS_IGNORE);
                        }

                        printf("Base station probed child reports with success: %d, %d\n", receivedFlag[0], receivedFlag[1]);

                        for (int i = 0; i < n_rows * m_cols; i++)
                        {

                                if (receivedFlag[i] == 1)
                                {

                                        struct MainSendReport MainRecReportMessage;

                                        MPI_Request mainPacketReq;
                                        MPI_Status mainPacketStat;

                                        struct timespec start, end;
                                        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

                                        // get the report structure from when a sensor has come to a local overheat consensus
                                        MPI_Recv(&MainRecReportMessage, 1, main_send_report_type, MPI_ANY_SOURCE, NODEBASEREPORT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                                        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
                                        u_int64_t t_delta = (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_nsec - start.tv_nsec) / 1000;

                                        fflush(stdout);
                                        printf("Master receiving overheat report\n");

                                        noOfMessages++;
                                        // all below is writing struct into log file and other important things
                                        // File keeping log for overheated nodes

                                        printf("---------------------------------------------------------------------------------------------------------------------------------------\n");
                                        printf("Base recieved adj node temps : [%d,%d,%d,%d]\n", MainRecReportMessage.adjNodesTemp[0], MainRecReportMessage.adjNodesTemp[1], MainRecReportMessage.adjNodesTemp[2], MainRecReportMessage.adjNodesTemp[3]);

                                        int r = getRow(MainRecReportMessage.nodeRank, m_cols), c = getCol(MainRecReportMessage.nodeRank, m_cols);
                                        int *adjR = getAdjRanks(MainRecReportMessage.nodeRank, n_rows, m_cols);
                                        int upR = getRow(adjR[0], m_cols), upC = getCol(adjR[0], m_cols);
                                        int downR = getRow(adjR[1], m_cols), downC = getCol(adjR[1], m_cols);
                                        int leftR = getRow(adjR[2], m_cols), leftC = getCol(adjR[2], m_cols);
                                        int rightR = getRow(adjR[3], m_cols), rightC = getCol(adjR[3], m_cols);

                                        pthread_mutex_lock(&satellite_Mutex);
                                        curr_satellite_temp = satellite_reports[MainRecReportMessage.nodeRank][0];
                                        curr_satellite_time = satellite_reports[MainRecReportMessage.nodeRank][1];
                                        pthread_mutex_unlock(&satellite_Mutex);

                                        sat_wsn_time_flag = 0;
                                        sat_wsn_temp_flag = 0;

                                        if (fabs(curr_satellite_time - MainRecReportMessage.reportTime) < 30)
                                        {
                                                sat_wsn_time_flag = 1;
                                        }

                                        if (abs(curr_satellite_temp - MainRecReportMessage.nodeTemp) < SATTHRESHOLDERROR)
                                        {
                                                sat_wsn_temp_flag = 1;
                                        }

                                        FILE *f = fopen("logfile.txt", "a");
                                        if (f == NULL)
                                        {
                                                printf("Error opening file!\n");
                                                exit(1);
                                        }

                                        // File keeping log of current state of network (2.d)
                                        FILE *f2 = fopen("performanceMetrics.txt", "a");
                                        if (f2 == NULL)
                                        {
                                                printf("Error opening file!\n");
                                                exit(1);
                                        }

                                        if (sat_wsn_temp_flag && sat_wsn_time_flag)
                                        {
                                                //Do Event Confirmed Log
                                                fprintf(f, "---------------------------------------------CONFIRMED REPORT ------------------------------------------------------------------------------------\n");
                                                fprintf(f, "Alert Type: True\n");
                                                confirmedReports ++;
                                        }
                                        else
                                        {
                                                // Do false report log
                                                fprintf(f, "-----------------------------------------------FALSE REPORT --------------------------------------------------------------------------------------\n");
                                                fprintf(f, "Alert Type: False\n");
                                                if (!sat_wsn_temp_flag)
                                                {
                                                        fprintf(f, "False Alert Error: Temp not within threshold of %d degrees\n", SATTHRESHOLDERROR);
                                                }
                                                if (!sat_wsn_time_flag)
                                                {
                                                        fprintf(f, "False Alert Error: Satellite Report not up to data\n");
                                                }
                                                falseReports ++;
                                        }

                                        time_t timer;
                                        char buff[26];
                                        struct tm *tm_info;

                                        timer = time(NULL);
                                        tm_info = localtime(&timer);

                                        strftime(buff, 26, "%Y-%m-%d %H:%M:%S", tm_info);

                                        // printing local overheat consensus data to logfile.txt
                                        fprintf(f, "Iterations: %d\n", i_iterations);
                                        fprintf(f, "Number of nodes compared with given sensor node: %d\n", MainRecReportMessage.nodesCompared);
                                        fprintf(f, "Time for receiving message from WSN: %lu millisecond\n", t_delta);
                                        fprintf(f, "Time of log: %s \n", buff);
                                        fprintf(f, "Local time (since the start of the program) when node in WSN reported local overheating consensus: %lf seconds\n", MainRecReportMessage.reportTime);
                                        fprintf(f, "The satellite recorded a temperature reading of %d at co-ordinate (%d, %d)\n", curr_satellite_temp, r, c);
                                        fprintf(f, "The satellite recorded this temperature at %d\n", curr_satellite_time);
                                        fprintf(f, "The node rank %d at co-ordinate (%d, %d) had a temperature reading of %d: \n", MainRecReportMessage.nodeRank, r, c, MainRecReportMessage.nodeTemp);
                                        fprintf(f, "This sensor node's MAC is %X:%X:%X:%X:%X:%X\n", MainRecReportMessage.nodeMAC[0], MainRecReportMessage.nodeMAC[1], MainRecReportMessage.nodeMAC[2], MainRecReportMessage.nodeMAC[3], MainRecReportMessage.nodeMAC[4], MainRecReportMessage.nodeMAC[5]);
                                        fprintf(f, "This sensor node's IP is %u.%u.%u.%u\n", MainRecReportMessage.nodeIP[0], MainRecReportMessage.nodeIP[1], MainRecReportMessage.nodeIP[2], MainRecReportMessage.nodeIP[3]);
                                        if (MainRecReportMessage.adjNodesTemp[0] != -1)
                                        {
                                                fprintf(f, "The up sensor node's MAC is %X:%X:%X:%X:%X:%X and IP is %u.%u.%u.%u at co-ordinate (%d, %d) with a temperature of %d C\n", MainRecReportMessage.adjNodesMAC[0], MainRecReportMessage.adjNodesMAC[1], MainRecReportMessage.adjNodesMAC[2], MainRecReportMessage.adjNodesMAC[3], MainRecReportMessage.adjNodesMAC[4], MainRecReportMessage.adjNodesMAC[5],
                                                        MainRecReportMessage.adjNodesIP[0], MainRecReportMessage.adjNodesIP[1], MainRecReportMessage.adjNodesIP[2], MainRecReportMessage.adjNodesIP[3], upR, upC, MainRecReportMessage.adjNodesTemp[0]);
                                        }
                                        if (MainRecReportMessage.adjNodesTemp[1] != -1)
                                        {
                                                fprintf(f, "The down sensor node's MAC is %X:%X:%X:%X:%X:%X and IP is %u.%u.%u.%u at co-ordinate (%d, %d) with a temperature of %d C\n", MainRecReportMessage.adjNodesMAC[6], MainRecReportMessage.adjNodesMAC[7], MainRecReportMessage.adjNodesMAC[8], MainRecReportMessage.adjNodesMAC[9], MainRecReportMessage.adjNodesMAC[10], MainRecReportMessage.adjNodesMAC[11],
                                                        MainRecReportMessage.adjNodesIP[4], MainRecReportMessage.adjNodesIP[5], MainRecReportMessage.adjNodesIP[6], MainRecReportMessage.adjNodesIP[7], downR, downC, MainRecReportMessage.adjNodesTemp[1]);
                                        }
                                        if (MainRecReportMessage.adjNodesTemp[2] != -1)
                                        {
                                                fprintf(f, "The left sensor node's MAC is %X:%X:%X:%X:%X:%X and IP is %u.%u.%u.%u at co-ordinate (%d, %d) with a temperature of %d C\n", MainRecReportMessage.adjNodesMAC[12], MainRecReportMessage.adjNodesMAC[13], MainRecReportMessage.adjNodesMAC[14], MainRecReportMessage.adjNodesMAC[15], MainRecReportMessage.adjNodesMAC[16], MainRecReportMessage.adjNodesMAC[17],
                                                        MainRecReportMessage.adjNodesIP[8], MainRecReportMessage.adjNodesIP[9], MainRecReportMessage.adjNodesIP[10], MainRecReportMessage.adjNodesIP[11], leftR, leftC, MainRecReportMessage.adjNodesTemp[2]);
                                        }
                                        if (MainRecReportMessage.adjNodesTemp[3] != -1)
                                        {
                                                fprintf(f, "The right sensor node's MAC is %X:%X:%X:%X:%X:%X and IP is %u.%u.%u.%u at co-ordinate (%d, %d) with a temperature of %d C\n", MainRecReportMessage.adjNodesMAC[18], MainRecReportMessage.adjNodesMAC[19], MainRecReportMessage.adjNodesMAC[20], MainRecReportMessage.adjNodesMAC[21], MainRecReportMessage.adjNodesMAC[22], MainRecReportMessage.adjNodesMAC[23],
                                                        MainRecReportMessage.adjNodesIP[12], MainRecReportMessage.adjNodesIP[13], MainRecReportMessage.adjNodesIP[14], MainRecReportMessage.adjNodesIP[15], rightR, rightC, MainRecReportMessage.adjNodesTemp[3]);
                                        }
                                        
                                        fprintf(f, "Number of false reports thus far (inclusive): %d & Number of confirmed reports thus far (inclusive): %d\n", falseReports, confirmedReports);

                                        // Writing other key metrics to performanceMetrics.txt

                                        fprintf(f2, "---------------------------------------------------------------------------------------------------------------------------------------\n");
                                        fprintf(f2, "Number of times a local overheating consensus of nodes has occurred thus far %d\n", noOfMessages);
                                        fprintf(f2, "Time of log: %s \n", buff);
                                        fprintf(f2, "current interval of the program is: %d seconds\n", v_interval);

                                        fclose(f);
                                        fclose(f2);
                                }
                        }
                        i_iterations--;
                }
                flag_run = 0;
        }
}
