#include "helper.hpp"
#include "sockets.hpp"

/* A struct used by the front-end to manage all the clients connected */
typedef struct
{
    bool free;
    ClientSocket *socket;
    int socket_id;
} str_clients_front;

int get_free_client();
int find_user_idx(int socket_id);
void close_client_conn(int socket_id);
void release_connection(int index);
bool create_server_connection(int idx, string username, string groupname);
void release_connection_by_id(int socket_id);
bool send_message(packet pkt, int socket_id);
void send_error_to_client(packet data, int socket_fd);