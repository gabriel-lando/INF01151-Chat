#ifndef __HELPER__
#define __HELPER__

// Headers
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
#include <pthread.h>
#include <termios.h>
#include <mutex>
#include <thread>
#include <ctime>
#include <sys/time.h>
#include <iomanip>
#include <signal.h>

using namespace std;

// constants
#define DEFAULT_PORT 6555
#define DEFAULT_HOSTNAME "localhost"
#define MAX_SIM_USR 2  // Max simultaneous users
#define MAX_CONNS 50   // Max client connections
#define CON_TIMEOUT 60 // 60 sec = 1 min

// functions
void error(string msg);
bool check_name(const string &name);
time_t get_time();
string get_timestamp(time_t time);

enum PktType
{
    PING,
    PONG,
    DATA,
    ACK,
    TIMEOUT,
    RECONNECTION,
    BACKUP
};

/**
 * Struct containing the format of the messages
 */
typedef struct __packet
{
    time_t timestamp;   // Data timestamp
    char username[21];  // Username
    char groupname[21]; // Group Name
    char message[100];  // Message data
    PktType type;
} packet;

typedef struct
{
    char ip[20];
    int port;
} server_info;

#endif