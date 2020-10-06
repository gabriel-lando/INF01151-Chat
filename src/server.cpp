#include "server.hpp"
#include "io.c"

std::mutex data_mtx, socket_mtx;

typedef struct {
    bool free;
    int socket_id;
    time_t last_msg;
    char group[50];
    char user[20];
} str_clients;

volatile str_clients clients[MAX_CONNS];

void InitClients(){
    data_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++){
        clients[i].free = true;
        clients[i].socket_id = 0;
        clients[i].last_msg = 0;
        strcpy((char*)clients[i].group, "");
        strcpy((char*)clients[i].user, "");
    }
    data_mtx.unlock();
}

int GetFreeClient(){
    for (int i = 0; i < MAX_CONNS; i++)
        if (clients[i].free)
            return i;
    return -1;
}

int FindUserIdx(int socket_id) {
    for (int i = 0; i < MAX_CONNS; i++)
        if (!clients[i].free && clients[i].socket_id == socket_id)
            return i;
    return -1;
}

int AddNewUser(packet pkt, int socket_id, bool *connected) {
    // Check if user is already connected
    int count = 0;
    for (int i = 0; i < MAX_CONNS; i++) {
        if (!clients[i].free && !strcmp((char*)clients[i].user, pkt.username))
            count++;
    }

    if (count >= 2)
        return -1;
    
    int idx = GetFreeClient();

    data_mtx.lock();
    clients[idx].free = false;
    clients[idx].socket_id = socket_id;
    clients[idx].last_msg = pkt.timestamp;
    strcpy((char*)clients[idx].group, pkt.groupname);
    strcpy((char*)clients[idx].user, pkt.username);
    data_mtx.unlock();

    *connected = count;
    return idx;
}

void WarnUsersFromDisconnection(string user, string group) {

    // Check if exists another connection from this user in this group
    for (int i = 0; i < MAX_CONNS; i++) {
        if (!clients[i].free)
            if (!strcmp((char*)clients[i].user, user.c_str()) && !strcmp((char*)clients[i].group, group.c_str()))
                return;
    }

    packet pkt;
    char msg[] = "<saiu do grupo>";
    strcpy(pkt.message, msg);

    strcpy(pkt.groupname, group.c_str());
    strcpy(pkt.username, user.c_str());
    pkt.timestamp = get_time();  

    std::thread write_message (SendMessage, pkt);
    write_message.detach();
}

void ReleaseConnection(int index){
    close(clients[index].socket_id);

    data_mtx.lock();
    string user = (char*)clients[index].user;
    string group = (char*)clients[index].group;
    clients[index].free = true;
    data_mtx.unlock();

    WarnUsersFromDisconnection(user, group);
}

void ReleaseConnectionByID(int socket_id){
    int idx = FindUserIdx(socket_id);
    if (idx != -1)
        ReleaseConnection(idx);
    else
        close(socket_id);
}

void UpdateUserConnection(packet pkt, int idx) {
    data_mtx.lock();
    clients[idx].last_msg = pkt.timestamp;
    data_mtx.unlock();
}

void CheckConnectionTimeout() {
    while (true) {
        time_t curr_time = get_time();

        for (int i = 0; i < MAX_CONNS; i++) {
            if (clients[i].free)
                continue;
            if (curr_time - clients[i].last_msg > CON_TIMEOUT){
                fprintf(stderr, "Client %s [ID %d] timed out.\n", clients[i].user, clients[i].socket_id);
                socket_mtx.lock();
                char c = '\0';
                int n = write(clients[i].socket_id, &c, sizeof(char));
                socket_mtx.unlock();
                ReleaseConnection(i);
            }
        }

        sleep(5);
    }
}

int WriteToSocket(packet pkt, int socket_id) {
    
    int n = write(socket_id, reinterpret_cast<char*>(&pkt), sizeof(packet));
    
    return n;
}

void LoadUserMessages(packet pkt, int socket_id) {

    int qty = atoi(pkt.message);
    if (!qty)
        return;

    string group = pkt.groupname;
    packet *response = (packet *)malloc(sizeof(packet) * qty);

    int read = LoadMessages(group, qty, response);

    if (!read)
        return;
    
    socket_mtx.lock();
    for (int i = 0; i < read; i++)
        WriteToSocket(response[i], socket_id);
    socket_mtx.unlock();

    return;
}

void SendMessage(packet pkt) {

    if(!SaveMessage(pkt)) {
        fprintf(stderr, "Error saving message\n");
        return;
    }
        

    socket_mtx.lock();
    for (int i = 0; i < MAX_CONNS; i++) {
        if (clients[i].free || strcmp((char*)clients[i].group, pkt.groupname))
            continue;

        int n = WriteToSocket(pkt, clients[i].socket_id);
        if (n <= 0)
            ReleaseConnection(i);
    }
    socket_mtx.unlock();
}

void ReceiveMessage(int socket_fd) {
    while (true) {
        packet data, buffer;
        bzero(&buffer, sizeof(packet));

        int n = read(socket_fd, &buffer, sizeof(packet));
        data = buffer;

        if (n != sizeof(packet)) {
            ReleaseConnectionByID(socket_fd);
            break;
        }

        bool was_connected = false;
        int idx = FindUserIdx(socket_fd);
        if (idx == -1) {
            idx = AddNewUser(data, socket_fd, &was_connected);

            if (idx != -1) {
                LoadUserMessages(data, socket_fd);
                strcpy(data.message, "<entrou no grupo>");
            }
        }

        if (idx != -1) {
            UpdateUserConnection(data, idx);

            if(!was_connected) {
                fprintf(stderr, "%s %s [%s]: %s\n", (get_timestamp(data.timestamp)).c_str(), data.username, data.groupname, data.message);
                std::thread write_message (SendMessage, data);
                write_message.detach();
            }
        }
        else {
            strcpy(data.message, "Connection Error");
            strcpy(data.username, "SERVER");

            socket_mtx.lock();
            int n = write(socket_fd, reinterpret_cast<char*>(&data), sizeof(packet));
            socket_mtx.unlock();

            ReleaseConnectionByID(socket_fd);
            break;
        }
    }
}

int main(int argc, char *argv[])
{   
    //Server *server = new Server(); 

    // Clear data structures to avoid memory trash
    InitClients();

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

    if (argc < 2)
        error("No port provided");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
       error("ERROR opening socket");

    // sets all values in a buffer to zero.
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, MAX_CONNS) != 0)
        error("ERROR on listening");

    std::cerr << "Server Started!" << endl;

    thread timeout (CheckConnectionTimeout);
    timeout.detach();

    while(true)
    {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                
        if (newsockfd < 0)
            error("ERROR on accept");

        std::cerr << "New connection" << endl;

        thread receive_msg (ReceiveMessage, newsockfd);
        receive_msg.detach();
    }
}