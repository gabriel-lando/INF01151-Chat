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

using namespace std;

// constants
#define DEFAULT_PORT 6555
#define DEFAULT_HOSTNAME "localhost"

// functions
void error(string msg);
bool check_name(const string &name);
time_t get_time();
string get_timestamp(time_t time);

typedef struct __packet
{
    time_t timestamp;   //Timestamp do dado
    char username[21];  //Nome do usuario
    char groupname[21]; //Nome do grupo
    char message[100];  //Dados da mensagem
} packet;

#endif