#include "helper.hpp"
#include "io.hpp"
#include "sockets.hpp"

#define MAX_BACKUPS 4

/* A struct used by the server to manage all the clients connected */
typedef struct
{
    bool free;
    int socket_id;
    time_t last_msg;
    char group[50];
    char user[20];
    bool isBackup;
} str_clients;

typedef struct
{
    bool free;
    int port;
} str_backups;

void send_message(packet pkt);
void receive_message(int socket_fd);