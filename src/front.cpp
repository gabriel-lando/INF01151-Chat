#include "front.hpp"

typedef struct
{
    bool connected;
    server_info server;
    int socket_id;
} str_server_addr;

std::mutex data_mtx, socket_mtx;

volatile str_clients_front clients[MAX_CONNS];
int qtde_msgs = 0;
volatile ClientSocket *server = nullptr;
volatile server_info srv_data = {"", 0};
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

/**
 * Retrieves the client from the client array with the socket_id provided
 * @param socket_id socket identifier
 */
int find_user_idx(int socket_id)
{
    for (int i = 0; i < MAX_CONNS; i++)
        if (!clients[i].free && clients[i].socket_id_client == socket_id)
            return i;
    return -1;
}

int find_client_id(int server_id)
{
    for (int i = 0; i < MAX_CONNS; i++)
        if (!clients[i].free && clients[i].server_socket->GetID() == server_id)
            return clients[i].socket_id_client;
    return -1;
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

void close_srv(int socket_id)
{
    char c = '\0';
    write(socket_id, &c, sizeof(char));
    close(socket_id);
}

void connect_to_server()
{
    if (server != nullptr && ((ClientSocket*)server)->IsConnected()) {
        return;
    }

    delete(server);

    ClientSocket *new_conn = nullptr;
    while (!create_new_connection(new_conn)) { usleep(1000); }
    server = new_conn;
    new_conn = nullptr;

    fprintf(stderr, "Reconnecting all clients to new server\n");

    for (int i = 0; i < MAX_CONNS; i++) {
        if (!clients[i].free) {
            create_new_connection(new_conn);
            if (new_conn->GetID() >= 0)
                clients[i].server_socket = new_conn;
            fprintf(stderr, "Reconnecting clients %d to server %d\n", clients[i].socket_id_client, clients[i].server_socket->GetID());
        }
    }
}

void receive_message_from_server(ClientSocket *socket)
{
    packet buffer;
    int response;

    while (socket->IsConnected())
    {
        fprintf(stderr, "Chegou aqui 0\n");
        if (!socket->ReceivePacket(&buffer, &response)) {
            socket_mtx.lock();
            if (!((ClientSocket*)server)->IsConnected())
                connect_to_server();
            socket_mtx.unlock();
        }

        int client_id = find_client_id(socket->GetID());
        fprintf(stderr, "Chegou aqui 1\n");
        
        /* Success receiving a message */
        if (response == sizeof(packet)) {
            send_message(buffer, client_id);
        }
        else {
            socket_mtx.lock();
            char c = '\0';
            int n = write(client_id, &buffer, response);
            socket_mtx.unlock();
            release_connection_by_id(client_id);
        }
    }
}

bool create_new_connection(ClientSocket *new_socket)
{
    if (((ClientSocket*)server)->IsConnected() == false)
        return false;
    
    new_socket = new ClientSocket((char*)srv_data.ip, srv_data.port);
    if(!new_socket->Connect()){
        delete(new_socket);
        return false;
    }
    return true;
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
    int n = write_to_socket(reinterpret_cast<void *>(&pkt), sizeof(packet), socket_id);
    socket_mtx.unlock();

    return (n == sizeof(packet));
}

bool send_message_as_client(packet pkt, ClientSocket *socket)
{
    /* Send a packet to a specific socket */
    socket_mtx.lock();
    int n = socket->SendData(reinterpret_cast<void *>(&pkt), sizeof(packet));
    socket_mtx.unlock();

    return (n == sizeof(packet));
}

/**
 * Client disconnects from the server
 * 
 * @param index index of the client array from the client that disconnected
 */
void release_connection(int index)
{
    close(clients[index].socket_id_client);
    if (clients[index].server_socket != nullptr)
        clients[index].server_socket->Disconnect();

    data_mtx.lock();
    clients[index].free = true;
    data_mtx.unlock();
}

/**
 * Uses the socket id to release client connection
 * 
 * @param socket_id_client socket identifier
 */
void release_connection_by_id(int socket_id_client)
{
    int idx = find_user_idx(socket_id_client);
    if (idx != -1)
        release_connection(idx);
    else
        close(socket_id_client);
}

/**
 * Function to write messages on socket
 * 
 * @param pkt  message to be written on socket
 * @param size message size
 * @param socket_id socket identifier
 */
int write_to_socket(void *pkt, int size, int socket_id)
{
    int n = write(socket_id, pkt, size);

    return n;
}

/**
 * Retrieves the client from the client array with the socket_id provided
 * @param pkt Struct that defines the messages sent by clients
 * @param socket_id socket identifier
 * @param connected a boolean to control if the client is already connected
 */
int add_new_user(packet pkt, int socket_id)
{
    ClientSocket server_conn((char *)srv_data.ip, srv_data.port);

    fprintf(stderr, "Chegou aqui -1\n");
    
    if(server_conn.Connect()){
        std::thread receive_thread(receive_message_from_server, &server_conn);
        receive_thread.detach();
    }

    if (server_conn.GetID() <= 0)
        connect_to_server();

    /* Since the client structure is a shared variable, it must be protected with mutex to maintain consistency*/
    data_mtx.lock();
    int idx = get_free_client();

    if (idx == -1)
    {
        data_mtx.unlock();
        server_conn.Disconnect();
        return -1;
    }

    clients[idx].free = false;
    clients[idx].socket_id_client = socket_id;
    clients[idx].server_socket = &server_conn;
    data_mtx.unlock();

    return idx;
}

void send_error_to_client(packet data, int socket_fd)
{
    strcpy(data.message, "Connection Error");
    strcpy(data.username, "SERVER");

    socket_mtx.lock();
    int n = send_message(data, socket_fd);
    socket_mtx.unlock();

    release_connection(socket_fd);
}

void manage_server(void *srv_info, int srv_id) {
    if (server != nullptr && ((ClientSocket*)server)->IsConnected()){
        socket_mtx.lock();
        int n = write_to_socket(srv_info, sizeof(server_info), srv_id);
        socket_mtx.unlock();

        close_srv(srv_id);
    }
    else {
        socket_mtx.lock();
        int n = write_to_socket('\0', sizeof(char), srv_id);
        socket_mtx.unlock();
        close_srv(srv_id);

        server_info *tmp = (server_info *)srv_info;

        if (server != nullptr)
            delete(server);

        usleep(1000);

        data_mtx.lock();
        server = new ClientSocket(tmp->ip, tmp->port);
        if(!((ClientSocket*)server)->Connect()){
            fprintf(stderr, "Error connecting to server: %s:%d\n", tmp->ip, tmp->port);
            delete(server);
            data_mtx.unlock();
            return;
        }
        strcpy((char*)srv_data.ip, tmp->ip);
        srv_data.port = tmp->port;
        data_mtx.unlock();
        fprintf(stderr, "New server connected: %s:%d\n", tmp->ip, tmp->port);
    }
}

void send_ping(int socket_fd)
{
    packet data;
    data.type = PktType::PING;
    send_message(data, socket_fd);
}

void respond_to_ping(int socket_fd)
{
    packet data;
    data.type = PktType::PONG;
    send_message(data, socket_fd);
}

void manage_client(int socket_fd)
{
    while (true)
    {
        packet data;
        bzero(&data, sizeof(packet));

        fprintf(stderr, "Chegou aqui -2\n");

        int n = read(socket_fd, &data, sizeof(packet));

        fprintf(stderr, "Chegou aqui -3\n");

        if (n != sizeof(packet))
        {
            if (n == sizeof(server_info)) {
                manage_server(reinterpret_cast<void *>(&data), socket_fd);
            }

            release_connection_by_id(socket_fd);
            break;
        }

        fprintf(stderr, "Chegou aqui -4\n");

        if(data.type == PktType::PING){
            respond_to_ping(socket_fd);
            continue;
        }
        else if(data.type == PktType::PONG){
            continue;
        }

        fprintf(stderr, "Chegou aqui -5\n");

        if (server == nullptr || !((ClientSocket*)server)->IsConnected()){
            send_error_to_client(data, socket_fd);
            break;
        }

        fprintf(stderr, "Chegou aqui -6\n");

        int idx = find_user_idx(socket_fd);
        if (idx == -1)
        {
            /* if user was not found on the clients array, add a new one */
            idx = add_new_user(data, socket_fd);
        }

        fprintf(stderr, "Chegou aqui -7\n");

        if (idx != -1)
        {
            std::thread write_message(send_message_as_client, data, clients[idx].server_socket);
            write_message.detach();
        }
        else
        {
            send_error_to_client(data, socket_fd);
            break;
        }
    }
}

void check_server_connection() {
    while(true) {
        usleep(PING_TIME);
        if (server != nullptr) {
            if (!((ClientSocket*)server)->IsConnected()){
                // Do something (connection error)
            }
        }

        /*if (!server.connected || server.socket_id <= 0)
            continue;

        send_ping(server.socket_id);*/
    }
}

int main(int argc, char *argv[])
{
    // Clear data structures to avoid memory trash
    init_clients();

    /*
        sockfd: file descriptor, will store values returned by socket system call
        newsockfd: file descriptor, will store values returned by accept system call
        portno: port number on which the server accepts connections
        cli_len: size of the address of the client 
    */
    int sockfd, newsockfd, portno, cli_len;

    /*
        serv_addr: address of the server
        cli_addr: address of the client connected
    */
    struct sockaddr_in serv_addr, cli_addr;

    if (argc != 2)
        error("Use: <port>");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    // sets all values in a buffer to zero
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, MAX_CONNS) != 0)
        error("ERROR on listening");

    std::cerr << "Front-end Started!" << endl;

    thread check_server(check_server_connection);

    while (true)
    {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)(&cli_len));

        if (newsockfd < 0)
            error("ERROR on accept");

        std::cerr << "New connection" << endl;

        thread new_client(manage_client, newsockfd);
        new_client.detach();
    }
}