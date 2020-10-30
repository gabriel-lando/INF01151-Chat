#include "helper.hpp"
#include "io.hpp"

class Server
{
public:
    int port;

private:
};

/* A struct used by the server to manage all the clients connected */
typedef struct
{
    bool free;
    int socket_id;
    time_t last_msg;
    char group[50];
    char user[20];
} str_clients;

void send_message(packet pkt);
void receive_message(int socket_fd);