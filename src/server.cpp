#include "server.hpp"

std::mutex data_mtx, socket_mtx;

volatile str_clients clients[MAX_CONNS];
volatile str_backups backups_table[MAX_BACKUPS];
volatile int backups_sockets[MAX_BACKUPS];
ServerSocket *server = nullptr;
int server_port = 0;
int qtde_msgs = 0;

/**
 * Function to initialize the str_clients structure with default values
 */
void init_data_structures()
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

    for (int i = 0; i < MAX_BACKUPS; i++) {
        backups_table[i].free = true;
        backups_table[i].port = 0;
        backups_sockets[i] = 0;
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


    /* Since the client structure is a shared variable, it must be protected with mutex to maintain consistency*/
    data_mtx.lock();
    int idx = get_free_client();

    if (idx == -1) {
        data_mtx.unlock();
        return -1;
    }

    clients[idx].free = false;
    clients[idx].socket_id = socket_id;

    clients[idx].last_msg = pkt.timestamp;
    strcpy((char *)clients[idx].group, pkt.groupname);
    strcpy((char *)clients[idx].user, pkt.username);

    data_mtx.unlock();

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
        server->DisconnectClient(socket_id);
}


void update_user_connection(packet pkt, int idx)
{
    data_mtx.lock();
    clients[idx].last_msg = pkt.timestamp;
    data_mtx.unlock();
}

/**
 * Function to write messages on socket
 * 
 * @param pkt  message to be written on socket
 * @param socket_id socket identifier
 */
int write_to_socket(packet pkt, int socket_id)
{
    int n = server->SendData(reinterpret_cast<char *>(&pkt),sizeof(packet), socket_id);
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

    socket_mtx.lock();
    packet response;
    memcpy(&response, &pkt, sizeof(packet));
    int bytes_received;
    for (int i = 1; i < MAX_BACKUPS; i++){
        if(backups_table[i].free)
            continue;
        write_to_socket(response, backups_sockets[i]);

        packet ack;
        do {
            if(!server->ReceivePacket(&ack, &bytes_received, backups_sockets[i])){
                if (bytes_received == sizeof(packet))
                    continue;
                socket_mtx.unlock();
                return;
            }
        } while (ack.type != PktType::ACK);
    }

    /* Writes the message to all clients connected. Using a mutex to maintain consistency of the messages */
    //socket_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++)
    {
        if (clients[i].free || strcmp((char *)clients[i].group, pkt.groupname))
            continue;

        int n = write_to_socket(pkt, clients[i].socket_id);
        if (n != sizeof(packet))
            release_connection(i);
    }
    socket_mtx.unlock();
}

void close_client_conn(int socket_id)
{
    socket_mtx.lock();
    char c = '\0';
    server->SendData(&c, sizeof(char), socket_id);
    server->DisconnectClient(socket_id);
    socket_mtx.unlock();
}

void add_backup(int socket_fd, int port) {

    data_mtx.lock();
    for (int i = 0; i < MAX_BACKUPS; i++) {
        if (!backups_table[i].free)
            continue;
        
        backups_table[i].free = false;
        backups_table[i].port = port;
        backups_sockets[i] = socket_fd;
        break;
    }

    for (int i = 0; i < MAX_BACKUPS; i++) {
        if (backups_table[i].free || backups_table[i].port == server_port)
            continue;
        server->SendData((void*)backups_table, sizeof(backups_table), backups_sockets[i]);
    }
    
    data_mtx.unlock();
    return;
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
        if (!server->ReceivePacket(&data, &bytes_read, socket_fd)){
            if (bytes_read == sizeof(packet))
                continue;

            close_client_conn(socket_fd);
            release_connection_by_id(socket_fd);
            break;
        }

        if (data.type == PktType::BACKUP) {
            add_backup(socket_fd, data.timestamp);
            break;
        }

        bool was_connected = false;
        int idx = find_user_idx(socket_fd);
        if (idx == -1) {
            /* if user was not found on the clients array, add a new one */
            idx = add_new_user(data, socket_fd, &was_connected);

            if (idx != -1 && data.type == PktType::DATA) {
                load_user_messages(data, socket_fd);
                strcpy(data.message, "<entrou no grupo>");
            }
        }

        if (idx != -1)
        {
            update_user_connection(data, idx);

            if (data.type == PktType::RECONNECTION)
                continue;

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

void im_a_backup(int my_port) {
    string server_ip = "127.0.0.1";
    ClientSocket backup(server_ip.c_str(), server_port);

    if (!backup.Connect())
        error("Error connecting to main server.");

    packet data;
    data.timestamp = my_port;
    data.type = PktType::BACKUP;
    if (backup.SendData(&data, sizeof(packet)) != sizeof(packet))
        error("Error sending info to main server.");

    int bytes_received;
    while (backup.IsConnected()) {
        if(!backup.ReceivePacket(&data, &bytes_received)) {
            if (bytes_received == sizeof(packet))
                continue;
            if (bytes_received == sizeof(backups_table)){
                str_backups *tmp_table = reinterpret_cast<str_backups*>(&data);

                for (int i = 0; i < MAX_BACKUPS; i++){
                    backups_table[i].free = tmp_table[i].free;
                    backups_table[i].port = tmp_table[i].port;
                }
            }
        }
        else {
            if (!save_message(data))
                error("Error saving message\n");
            else {
                data.type = PktType::ACK;
                backup.SendData(&data, sizeof(packet));
            }
        }
    }
    backup.Disconnect();

    // Decide wich backup will be the new server
    int max_port = my_port;
    for (int i = 1; i < MAX_BACKUPS; i++){
        if (!backups_table[i].free && backups_table[i].port > max_port)
            max_port = backups_table[i].port;
    }

    if (max_port == my_port)
        return;

    sleep(1);
    im_a_backup(my_port);

    //error("Error communicating with main server.");
}

/**
 * Funtion with main loop of the server
 * The server should establish it's connection with the following parameters:
 * <port> <number_of_messages_to_save>
 */
int main(int argc, char *argv[])
{
    // Change SIGPIPE from socket server to SIG_IGN to avoid crashes 
    signal(SIGPIPE, SIG_IGN);

    // Clear data structures to avoid memory trash
    init_data_structures();

    if (argc < 3 || argc > 4)
        error("Use: [main_server_port] [num_msgs] <bkp_srv_port> ");
    
    server_port = atoi(argv[1]);
    qtde_msgs = atoi(argv[2]);

    if (argc == 4){
        fprintf(stderr, "It's a backup :)\n");
        int my_port = atoi(argv[3]);
        im_a_backup(my_port);
    }

    init_data_structures();

    backups_table[0].free = false;
    backups_table[0].port = server_port;
    fprintf(stderr, "New server :)\n");

    usleep(50000);
    server = new ServerSocket(server_port);
    switch(server->Start())
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

    /*thread timeout(check_connection_timeout);
    timeout.detach();*/

    int newsockfd;
    while (newsockfd = server->WaitNewConnection())
    {
        if (newsockfd < 0) {
            server->Disconnect();
            error("ERROR on accept");
        }

        std::cerr << "New connection: " << newsockfd << endl;

        thread receive_msg(receive_message, newsockfd);
        receive_msg.detach();
    }

    server->Disconnect();
}