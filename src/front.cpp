#include "front.hpp"

const string server_ip = "127.0.0.1";
int server_port = 0;

std::mutex data_mtx, socket_mtx;

volatile str_clients_front clients[MAX_CONNS];
ServerSocket *front = nullptr;

/**
 * Function to initialize the str_clients structure with default values
 */
void init_clients()
{
    data_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++)
        clients[i].free = true;
    data_mtx.unlock();
}

void receive_message_from_server(int idx, string username, string groupname)
{
    try {
        packet buffer;
        int bytes_read;
        int client_id = clients[idx].socket_id;
        ClientSocket *socket = clients[idx].socket;

        while (socket->IsConnected())
        {
            if (!socket->ReceivePacket(&buffer, &bytes_read)) {
                if (bytes_read == sizeof(packet))
                    continue;
                // Try to reconnect with server
                data_mtx.lock();
                if (clients[idx].free == false) {
                    socket_mtx.lock();
                    while (!create_server_connection(idx, username, groupname)) { sleep(1); }
                    socket_mtx.unlock();

                    packet pkt;
                    pkt.timestamp = get_time();
                    pkt.type = PktType::RECONNECTION;
                    strcpy(pkt.groupname, groupname.c_str());
                    strcpy(pkt.username, username.c_str());

                    socket_mtx.lock();
                    clients[idx].socket->SendData(&pkt, sizeof(packet));
                    socket_mtx.unlock();
                }
                data_mtx.unlock();
                break;
            }

            /* Success receiving a message */
            if (bytes_read == sizeof(packet)) {
                send_message(buffer, client_id);
            }
            else {
                close_client_conn(client_id);
                release_connection(idx);
            }
        }
    }
    catch(...) { }
}

bool create_server_connection(int idx, string username, string groupname) {
    try {
        clients[idx].socket = new ClientSocket(server_ip.c_str(), server_port);
        
        if(clients[idx].socket->Connect()) {
            std::thread receive_thread(receive_message_from_server, idx, username, groupname);
            receive_thread.detach();
            return true;
        }
        else {
            clients[idx].socket->Disconnect();
        }
    } catch(...) { }
    return false;
}

/**
 * Retrieves the client from the client array with the socket_id provided
 * @param pkt Struct that defines the messages sent by clients
 * @param socket_id socket identifier
 * @param connected a boolean to control if the client is already connected
 */
int add_new_user(packet pkt, int socket_id)
{
    /* Since the client structure is a shared variable, it must be protected with mutex to maintain consistency*/
    data_mtx.lock();
    int idx = get_free_client();

    if (idx == -1) {
        data_mtx.unlock();
        return -1;
    }

    if (!create_server_connection(idx, pkt.username, pkt.groupname)){
        data_mtx.unlock();  
        return -1;
    }

    clients[idx].free = false;
    clients[idx].socket_id = socket_id;
    data_mtx.unlock();

    return idx;
}


void manage_client(int socket_fd)
{
    packet data;
    int bytes_read;

    while (true)
    {
        if (!front->ReceivePacket(&data, &bytes_read, socket_fd)) {
            if (bytes_read == sizeof(packet))
                continue;
            close_client_conn(socket_fd);
            release_connection_by_id(socket_fd);
            break;
        }

        int idx = find_user_idx(socket_fd);
        if (idx == -1) {
            /* if user was not found on the clients array, add a new one */
            idx = add_new_user(data, socket_fd);
        }

        if (idx != -1) {
            int n;
            socket_mtx.lock();
            try{
                n = clients[idx].socket->SendData((void*)(&data), sizeof(packet));
            }
            catch(...) { }
            socket_mtx.unlock();

            if (n != sizeof(packet))
                fprintf(stderr, "Server connection error\n");
        }
        else {
            send_error_to_client(data, socket_fd);
            break;
        }
    }
}


int main(int argc, char *argv[])
{
    // Change SIGPIPE from socket server to SIG_IGN to avoid crashes 
    signal(SIGPIPE, SIG_IGN);

    // Clear data structures to avoid memory trash
    init_clients();

    if (argc != 3)
        error("Use: <front_port> <server_port>");

    int portno = atoi(argv[1]);
    server_port = atoi(argv[2]);

    front = new ServerSocket(portno);
    switch(((ServerSocket*)front)->Start())
    {
        case -1:
            error("ERROR opening socket"); break;
        case -2:
            error("ERROR on binding"); break;
        case -3:
            error("ERROR on listening"); break;
        default:
            break;
    }

    std::cerr << "Front-end Started!" << endl;

    int newsockfd;
    while (newsockfd = front->WaitNewConnection())
    {
        if (newsockfd < 0) {
            front->Disconnect();
            error("ERROR on accept");
        }

        std::cerr << "New connection: " << newsockfd << endl;

        thread new_client(manage_client, newsockfd);
        new_client.detach();
    }

    front->Disconnect();

    std::cerr << "Front-end Shutting down!" << endl;
}

/**
 * Sends a pkt message to all clients connected, and saves the message to a file
 * 
 * @param pkt  packet message
 * @param socket_id socket id to send the packet
 */
bool send_message(packet pkt, int socket_id)
{
    /* Send a packet to a specific socket */
    socket_mtx.lock();
    int n = front->SendData(reinterpret_cast<void *>(&pkt), sizeof(packet), socket_id);
    socket_mtx.unlock();

    return (n == sizeof(packet));
}

void send_error_to_client(packet data, int socket_fd)
{
    strcpy(data.message, "Connection Error");
    strcpy(data.username, "SERVER");

    socket_mtx.lock();
    send_message(data, socket_fd);
    socket_mtx.unlock();

    release_connection_by_id(socket_fd);
}

/**
 * Retrieves the first free client from the array str_clients
 */
int get_free_client()
{
    for (int i = 0; i < MAX_CONNS; i++)
        if (clients[i].free)
            return i;
    return -1;
}

void close_client_conn(int socket_id)
{
    socket_mtx.lock();
    char c = '\0';
    front->SendData(&c, sizeof(char), socket_id);
    front->DisconnectClient(socket_id);
    socket_mtx.unlock();
}


/**
 * Retrieves the client from the client array with the socket_id provided
 * @param socket_id socket identifier
 */
int find_user_idx(int socket_id)
{
    for (int i = 0; i < MAX_CONNS; i++)
        if (!clients[i].free && clients[i].socket_id == socket_id)
            return i;
    return -1;
}

/**
 * Client disconnects from the server
 * 
 * @param index index of the client array from the client that disconnected
 */
void release_connection(int index)
{
    data_mtx.lock();
    clients[index].free = true;
    
    close_client_conn(clients[index].socket_id);
    if (clients[index].socket != nullptr){
        try{
            clients[index].socket->Disconnect();
        } catch(...) { }
    }

    clients[index].socket = nullptr;
    data_mtx.unlock();
}

/**
 * Uses the socket id to release client connection
 * 
 * @param socket_id_client socket identifier
 */
void release_connection_by_id(int socket_id)
{
    int idx = find_user_idx(socket_id);
    if (idx != -1)
        release_connection(idx);
    else
        close_client_conn(socket_id);
}