#include "helper.hpp"

#define MAX_CONNS 50
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