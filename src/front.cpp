#include "front.hpp"

typedef struct
{
    bool connected;
    server_info server;
} str_server_addr;

std::mutex data_mtx, socket_mtx;

volatile str_clients_front clients[MAX_CONNS];
volatile str_server_addr server = {false, "", 0};
int qtde_msgs = 0;

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
        if (!clients[i].free && clients[i].socket_id_server == server_id)
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

void connect_to_server()
{
    if (server.connected) {
        int id = connect_client_to_server();
        if (id >= 0) {
            close(id);
            return;
        }

        server.connected = false;
    }

    while (connect_client_to_server() == -1) {
        usleep(1000);
    }

    fprintf(stderr, "Reconnecting all clients to new server\n");

    for (int i = 0; i < MAX_CONNS; i++) {
        if (!clients[i].free) {
            int id = connect_client_to_server();
            if (id >= 0)
                clients[i].socket_id_server = id;
            fprintf(stderr, "Reconnecting clients %d to server %d\n", clients[i].socket_id_client, clients[i].socket_id_server);
        }
    }


}

void receive_message(int socket_fd)
{
    packet buffer;

    while (true)
    {
        bzero(&buffer, sizeof(packet));
        int response = recvfrom(socket_fd, &buffer, sizeof(packet), 0, NULL, NULL);

        if (response >= 0){
            socket_mtx.lock();
            connect_to_server();
            socket_mtx.unlock();
        }

        int client_id = find_client_id(socket_fd);
        
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

int connect_client_to_server()
{
    if (server.connected == false)
        return -1;
    
    struct sockaddr_in serv_addr, cli_addr;
    char server_ip[20];
    int server_port;

    data_mtx.lock();
    strcpy(server_ip, (char*)server.server.ip);
    server_port = server.server.port;
    data_mtx.unlock();

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        return -1;

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        return -1;

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        return -1;

    std::thread receive_thread(receive_message, sockfd);
    receive_thread.detach();
    
    return sockfd;
}

/**
 * Sends a pkt message to all clients connected, and saves the message to a file
 * 
 * @param pkt  packet message
 */
bool send_message(packet pkt, int socket_id)
{
    /* Send a packet to a specific socket */
    socket_mtx.lock();
    int n = write_to_socket(reinterpret_cast<void *>(&pkt), sizeof(packet), socket_id);
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
    close(clients[index].socket_id_server);

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
    /* Since the client structure is a shared variable, it must be protected with mutex to maintain consistency*/
    int server_id = connect_client_to_server();
    if (server_id == -1)
    {
        //Reconnect to server
    }

    data_mtx.lock();
    int idx = get_free_client();

    if (idx == -1)
    {
        data_mtx.unlock();
        close(server_id);
        return -1;
    }

    clients[idx].free = false;
    clients[idx].socket_id_client = socket_id;
    clients[idx].socket_id_server = server_id;
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
    if (server.connected){
        socket_mtx.lock();
        int n = write_to_socket(srv_info, sizeof(server_info), srv_id);
        socket_mtx.unlock();
    }
    else {
        server_info *tmp = (server_info *)srv_info;

        data_mtx.lock();
        server.connected = true;
        strcpy((char*)server.server.ip, tmp->ip);
        server.server.port = tmp->port;
        data_mtx.unlock();
    }

    fprintf(stderr, "New server connected: %s:%d\n", server.server.ip, server.server.port);
    close(srv_id);
}

void manage_client(int socket_fd)
{
    while (true)
    {
        packet data, buffer;
        bzero(&buffer, sizeof(packet));

        int n = read(socket_fd, &buffer, sizeof(packet));
        data = buffer;

        if (n != sizeof(packet))
        {
            if (n == sizeof(server_info)) {
                manage_server(reinterpret_cast<void *>(&data), socket_fd);
            }

            release_connection_by_id(socket_fd);
            break;
        }

        if (!server.connected){
            send_error_to_client(data, socket_fd);
            break;
        }

        int idx = find_user_idx(socket_fd);
        if (idx == -1)
        {
            /* if user was not found on the clients array, add a new one */
            idx = add_new_user(data, socket_fd);
        }

        if (idx != -1)
        {
            std::thread write_message(send_message, data, clients[idx].socket_id_server);
            write_message.detach();
        }
        else
        {
            send_error_to_client(data, socket_fd);
            break;
        }
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

    // n contains number of characteres read or written
    int n;

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