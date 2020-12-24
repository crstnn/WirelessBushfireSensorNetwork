#ifndef SIMULATOR_H
#define SIMULATOR_H

//Include Libraries
#include <mpi.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> //strdup
#include <pthread.h>
#include <errno.h>
#include <unistd.h> //opts
#include <sys/stat.h>
//
#include "utils.h"

//Definitions
#define VERSION 0.1
#define THRESHOLDTEMP 80
#define ADJTHRESHOLDERROR 5
#define SATTHRESHOLDERROR 10
#define RANDSEED 89762123
//MPI COMM TAGS
#define BASENODEHBEAT 0
#define NODENODEHBEAT 1
#define BASENODESHUTDOWN 2
#define NODENODETEMPREQ 3
#define NODENODETEMPREPLY 4
#define NODEBASEREPORT 5
#define NOCHARSTOREAD 1

//Structs

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

// Initialising struct packet sent between sensor nodes
struct NeighbourPacket
{
        int nodeRank;
        int nodeTemp;
        unsigned char nodeIP[4];
        unsigned char nodeMAC[6];
};

#endif