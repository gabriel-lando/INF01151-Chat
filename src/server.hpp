#include "helper.hpp"
#include "io.hpp"

#define MAX_SIM_USR 2  // Max simultaneous users
#define MAX_CONNS 50   // Max client connections
#define CON_TIMEOUT 60 // 60 sec = 1 min

class Server
{
public:
    int port;

    void handle_communication(int sock);

private:
};

void SendMessage(packet pkt);
void ReceiveMessage(int socket_fd);