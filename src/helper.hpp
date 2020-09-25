// Headers
#include <stdio.h>
#include <sys/types.h> 
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

using namespace std;

// constants
#define DEFAULT_PORT 6555
#define DEFAULT_HOSTNAME "localhost"

// functions 
void error(string msg);
bool check_name(const string &name);