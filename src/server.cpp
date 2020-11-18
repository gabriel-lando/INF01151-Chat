#include "server.hpp"

std::mutex data_mtx, socket_mtx;

volatile str_clients clients[MAX_CONNS];
volatile ServerSocket *server = nullptr;
int qtde_msgs = 0;

/**
 * Function to initialize the str_clients structure with default values
 */
void init_clients()
{
    data_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++)
    {
        clients[i].free = true;
        clients[i].socket_id = 0;
        clients[i].last_msg = 0;
        clients[i].isBackup = false;
        strcpy((char *)clients[i].group, "");
        strcpy((char *)clients[i].user, "");
    }
    data_mtx.unlock();
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
 * Retrieves the client from the client array with the socket_id provided
 * @param pkt Struct that defines the messages sent by clients
 * @param socket_id socket identifier
 * @param connected a boolean to control if the client is already connected
 */
int add_new_user(packet pkt, int socket_id, bool *connected)
{
    // Check if user is already connected
    if (pkt.type == PktType::DATA){
        int count = 0;
        for (int i = 0; i < MAX_CONNS; i++)
        {
            if (!clients[i].free && !strcmp((char *)clients[i].user, pkt.username))
            {
                count++;
                if (!strcmp((char *)clients[i].group, pkt.groupname))
                    *connected = true;
            }
        }

        /* Server does not allow more than two instances of the same client connected */
        if (count >= MAX_SIM_USR)
            return -1;
    }


    /* Since the client structure is a shared variable, it must be protected with mutex to maintain consistency*/
    data_mtx.lock();
    int idx = get_free_client();

    if (idx == -1)
    {
        data_mtx.unlock();
        return -1;
    }

    clients[idx].free = false;
    clients[idx].socket_id = socket_id;

    if (pkt.type == PktType::DATA) {
        clients[idx].last_msg = pkt.timestamp;
        strcpy((char *)clients[idx].group, pkt.groupname);
        strcpy((char *)clients[idx].user, pkt.username);
    }
    else
        clients[idx].isBackup = true;

    data_mtx.unlock();

    if (pkt.type == PktType::BACKUP)
        return -2;

    return idx;
}

/**
 * Sends a message to warn the connected clients about a client disconnection
 * 
 * @param user user name
 * @param group group name
 */
void warn_users_from_disconnection(string user, string group)
{
    // Check if exists another connection from this user in this group, to avoid sending disconnection message if there is more than one instance of the same client
    for (int i = 0; i < MAX_CONNS; i++)
    {
        if (!clients[i].free)
            if (!strcmp((char *)clients[i].user, user.c_str()) && !strcmp((char *)clients[i].group, group.c_str()))
                return;
    }

    packet pkt;
    char msg[] = "<saiu do grupo>";
    strcpy(pkt.message, msg);

    strcpy(pkt.groupname, group.c_str());
    strcpy(pkt.username, user.c_str());
    pkt.timestamp = get_time();

    /* Uses a thread to send the messages */
    std::thread write_message(send_message, pkt);
    write_message.detach();
}

/**
 * Client disconnects from the server
 * 
 * @param index index of the client array from the client that disconnected
 */
void release_connection(int index)
{
    //close(clients[index].socket_id);
    ((ServerSocket*)server)->DisconnectClient(clients[index].socket_id);

    data_mtx.lock();
    string user = (char *)clients[index].user;
    string group = (char *)clients[index].group;
    clients[index].free = true;
    clients[index].isBackup = false;
    data_mtx.unlock();

    warn_users_from_disconnection(user, group);
}

/**
 * Uses the socket id to release client connection
 * 
 * @param socket_id socket identifier
 */
void release_connection_by_id(int socket_id)
{
    int idx = find_user_idx(socket_id);
    if (idx != -1)
        release_connection(idx);
    else
        //close(socket_id);
        ((ServerSocket*)server)->DisconnectClient(socket_id);
}


void update_user_connection(packet pkt, int idx)
{
    data_mtx.lock();
    clients[idx].last_msg = pkt.timestamp;
    data_mtx.unlock();
}

/**
 * Function to controls the idleness of clients 
 * 
 * A thread verifies periodically if the time of the last message of the clients is greater than CON_TIMEOUT
 */
void check_connection_timeout()
{
    while (true)
    {
        time_t curr_time = get_time();

        for (int i = 0; i < MAX_CONNS; i++)
        {
            if (clients[i].free || clients[i].isBackup)
                continue;
            if (curr_time - clients[i].last_msg > CON_TIMEOUT)
            {
                fprintf(stderr, "Client %s [ID %d] timed out.\n", clients[i].user, clients[i].socket_id);
                socket_mtx.lock();
                char c = '\0';
                int n = write(clients[i].socket_id, &c, sizeof(char));
                socket_mtx.unlock();
                release_connection(i);
            }
        }

        sleep(5);
    }
}

/**
 * Function to write messages on socket
 * 
 * @param pkt  message to be written on socket
 * @param socket_id socket identifier
 */
int write_to_socket(packet pkt, int socket_id)
{
    int n = ((ServerSocket*)server)->SendData(reinterpret_cast<char *>(&pkt),sizeof(packet), socket_id);
    //int n = write(socket_id, reinterpret_cast<char *>(&pkt), sizeof(packet));
    return n;
}

/**
 * Loads all the messages from a binary file related to the client group and write them on the socket and console
 * 
 * @param pkt  packet message
 * @param socket_id socket identifier
 */
void load_user_messages(packet pkt, int socket_id)
{

    if (!qtde_msgs)
        return;

    string group = pkt.groupname;
    packet *response = (packet *)malloc(sizeof(packet) * qtde_msgs);

    int read = load_messages(group, qtde_msgs, response);

    if (!read)
        return;

    /* mutex to maintein consistency of the socker buffer */
    socket_mtx.lock();
    for (int i = 0; i < read; i++)
        write_to_socket(response[i], socket_id);
    socket_mtx.unlock();

    return;
}

/**
 * Sends a pkt message to all clients connected, and saves the message to a file
 * 
 * @param pkt  packet message
 */
void send_message(packet pkt)
{

    /* Every time a message is sent, saves to the corresponding binary group file */
    if (!save_message(pkt))
    {
        fprintf(stderr, "Error saving message\n");
        return;
    }

    /* Writes the message to all clients connected. Using a mutex to maintain consistency of the messages */
    socket_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++)
    {
        if (clients[i].free || (strcmp((char *)clients[i].group, pkt.groupname) && !clients[i].isBackup))
            continue;

        int n = write_to_socket(pkt, clients[i].socket_id);
        if (n != sizeof(packet))
            release_connection(i);
    }
    socket_mtx.unlock();
}

/**
 * Funtion with a loop to keep receiving messages from the connected clients
 * 
 * @param socket_fd  file descriptor, will store values returned by socket system call
 */
void receive_message(int socket_fd)
{
    packet data;
    int bytes_read;

    while (true)
    {
        bool result = ((ServerSocket*)server)->ReceivePacket(&data, &bytes_read, socket_fd);

        if (!result)
        {
            if (bytes_read == sizeof(packet))
                continue;

            release_connection_by_id(socket_fd);
            break;
        }

        bool was_connected = false;
        int idx = find_user_idx(socket_fd);
        if (idx == -1)
        {
            /* if user was not found on the clients array, add a new one */
            idx = add_new_user(data, socket_fd, &was_connected);

            if (idx == -2)
                continue;

            if (idx != -1)
            {
                load_user_messages(data, socket_fd);
                strcpy(data.message, "<entrou no grupo>");
            }
        }

        if (idx != -1)
        {
            update_user_connection(data, idx);

            if (!was_connected)
            {
                fprintf(stderr, "%s %s [%s]: %s\n", (get_timestamp(data.timestamp)).c_str(), data.username, data.groupname, data.message);
                std::thread write_message(send_message, data);
                write_message.detach();
            }
        }
        else
        {
            strcpy(data.message, "Connection Error");
            strcpy(data.username, "SERVER");

            socket_mtx.lock();
            int n = write_to_socket(data, socket_fd);
            socket_mtx.unlock();

            release_connection(socket_fd);
            break;
        }
    }
}

int send_ip_to_front(char *front_ip, int front_port, server_info data, server_info *resp)
{
    ClientSocket front(front_ip, front_port);
    if (!front.Connect(false))
        return -1;

    int bytes_written = front.SendData(reinterpret_cast<void *>(&data), sizeof(server_info));

    if (bytes_written != sizeof(server_info))
        return -1;

    bzero(resp, sizeof(server_info));
    int bytes_read;
    front.ReceiveData(resp, sizeof(server_info), &bytes_read);
    front.Disconnect();

    if (bytes_read == 0)
        return -1;
    
    if (bytes_read == sizeof(server_info)){
        fprintf(stderr, "Main server: %s:%d\n", resp->ip, resp->port);
        return 0;
    }

    return 1;
}

void connect_to_main_server(server_info srv_info)
{
    ClientSocket client(srv_info.ip, srv_info.port);
    if (!client.Connect())
        return;
    
    packet data = {0, "", "", "", PktType::BACKUP};

    int bytes_written = client.SendData(reinterpret_cast<void *>(&data), sizeof(packet));
    if (bytes_written != sizeof(packet))
        return;
    
    int bytes_read;

    while (client.IsConnected())
    {
        bool result = client.ReceivePacket(&data, &bytes_read);

        if (!result)
        {
            if (bytes_read == sizeof(packet) && data.type != PktType::DATA)
                continue;
            break;
        }

        fprintf(stderr, "%s %s [%s]: %s\n", (get_timestamp(data.timestamp)).c_str(), data.username, data.groupname, data.message);

        if (!save_message(data))
            error("Error saving message\n");
    }
}

/**
 * Funtion with main loop of the server
 * The server should establish it's connection with the following parameters:
 * <port> <number_of_messages_to_save>
 */
int main(int argc, char *argv[])
{
    // Clear data structures to avoid memory trash
    init_clients();

    if (argc < 5 || argc > 6)
        error("Use: <server_ip> <server_port> <front_ip> <front_port> <N>");

    int front_ip = atoi(argv[4]);
    server_info srv_info, srv_resp;
    strcpy(srv_info.ip, argv[1]);
    srv_info.port = atoi(argv[2]);
    int resp = 0;

    do {
        resp = send_ip_to_front(argv[3], front_ip, srv_info, &srv_resp);
        if (resp < 0)
            error("Front unavailable");
        if (resp == 0) {
            connect_to_main_server(srv_resp);
            sleep(1);
        }
    } while (resp <= 0);

    if (argc == 6)
        qtde_msgs = atoi(argv[5]);

    int portno = atoi(argv[2]);

    server = new ServerSocket(portno);
    switch(((ServerSocket*)server)->Start())
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

    std::cerr << "Server Started!" << endl;

    thread timeout(check_connection_timeout);
    timeout.detach();

    int newsockfd;

    while (newsockfd = ((ServerSocket*)server)->WaitNewConnection())
    {
        if (newsockfd < 0) {
            ((ServerSocket*)server)->Disconnect();
            error("ERROR on accept");
        }

        std::cerr << "New connection: " << newsockfd << endl;

        thread receive_msg(receive_message, newsockfd);
        receive_msg.detach();
    }

    ((ServerSocket*)server)->Disconnect();
}