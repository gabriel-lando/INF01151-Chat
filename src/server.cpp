#include "server.hpp"

std::mutex mtx;

/*typedef struct {
    int socket_id;
    packet content;
} message_str;*/

typedef struct _sockets_id {
    int id;
    time_t timestamp;
    struct _sockets_id *next;
} sockets_id;

typedef struct _groups {
    string groupname;
    sockets_id *id;
    struct _groups *next;
} groups;

groups *listGroups = NULL;

sockets_id * GetIDs(string groupname){
    if (listGroups == NULL)
        return NULL;

    cerr << "GET IDs name: " << groupname << endl;

    groups *tmp = listGroups;
    while (tmp != NULL){
        if (tmp->groupname == groupname){

            sockets_id *ids = tmp->id;
            while (ids != NULL){
                cerr << "GET IDs: " << ids->id << endl;
                ids = ids->next;
            }

            return tmp->id;
        }
        tmp = tmp->next;
    }
    return NULL;
}

groups *InsertSocketID(groups *group, int id) {
    sockets_id *tmp = group->id;

    while (tmp != NULL){
        if (tmp->id == id)
            break;
        tmp = tmp->next;
    }

    if (tmp == NULL) {
        tmp = (sockets_id*)malloc(sizeof(sockets_id));
        tmp->id = id;
        tmp->next = group->id;
    }

    cerr << "Insert ID: " << id << " ID: " << tmp->id << endl;

    group->id = tmp;
    return group;
}

void RemoveIdFromGroup(string groupname, int socket_id) {
    if (listGroups == NULL)
        return;
    
    groups *prev = listGroups, *curr = listGroups;

    cerr << "Remove Group: " << groupname << " ID: " << socket_id << endl;

    while (curr != NULL) {
        if (curr->groupname == groupname){
            sockets_id *prev_id = curr->id, *curr_id = curr->id;
            while (curr_id != NULL) {
                if (curr_id->id == socket_id) {
                    prev_id->next = curr_id->next;
                    free(curr_id);
                    break;
                }

                prev_id = curr_id;
                curr_id = curr_id->next;
            }

            if (curr->id == NULL) {
                prev->next = curr->next;
                free(curr);
                break;
            }
        }

        prev = curr;
        curr = curr->next;
    }
}

void InsertGroup(string groupname, int socket_id){
    groups *tmp = listGroups;

    while (tmp != NULL){
        if (tmp->groupname == groupname){
            cerr << "Find group: " << tmp->groupname << endl;
            tmp = InsertSocketID(tmp, socket_id);
            cerr << "Inserted ID: " << socket_id << endl;
            break;
        }
        tmp = tmp->next;
    }

    if (tmp == NULL) {
        tmp = (groups*)malloc(sizeof(groups));
        tmp->next = listGroups;
        tmp->id = NULL;
        tmp->groupname = groupname;
        tmp = InsertSocketID(tmp, socket_id);

        listGroups = tmp;
    }

    cerr << "Group: " << tmp->groupname << " ID: " << tmp->id->id << endl;
}

void RemoveID(int id) {
    if (listGroups == NULL)
        return;

    cerr << "Removing only ID: " << id << endl;

    groups *tmp = listGroups;
    while (tmp != NULL){
        RemoveIdFromGroup(tmp->groupname, id);
        tmp = tmp->next;
    }
}


/*
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
*/
/*void Server::handle_communication (int sock)
{
    int n;
    char buffer[sizeof(packet)];
      
    bzero(buffer, sizeof(packet));
    n = read(sock, buffer, sizeof(packet));

    if (n < 0) 
        error("ERROR reading from socket");

    packet *pkt = (packet*)buffer;
        
    printf("Here is the message: %s\n", pkt->message);
    n = write(sock, buffer, sizeof(packet));

    if (n < 0)
        error("ERROR writing to socket");

    close(sock);
}*/

void * SendMessage(void * data) {
    packet content = *(packet*) data;

    sockets_id *ids = GetIDs(string(content.groupname));
    while (ids != NULL) {
        int current_id = ids->id;
        cout << "Send to ID: " << current_id << endl;

        mtx.lock();
        int n = write(current_id, reinterpret_cast<char*>(&content), sizeof(packet));
        mtx.unlock();

        ids = ids->next;

        if (n <= 0)
            RemoveIdFromGroup(string(content.groupname), current_id);
    }
}

void * ReceiveMessage(void * socket) {
    int socket_fd, n;
    socket_fd = *(int *) socket;

    cerr << "socket_fd: " << socket_fd << endl;

    while(true) {
        packet data, buffer;
        bzero(&buffer, sizeof(packet));

        n = read(socket_fd, &buffer, sizeof(packet));
        data = buffer;

        cerr << "Packet size: " << n << endl;

        if (n < 0) 
            error("ERROR reading from socket");
        else if (n == 0){
            RemoveID(socket_fd);
            break;
        }

        if (strlen(data.groupname)) {
            string groupname = string(data.groupname);
            InsertGroup(groupname, socket_fd);

            printf("%s %s [%s]: %s\n", (get_timestamp(data.timestamp)).c_str(), data.username, data.groupname, data.message);

            pthread_t tid;
            pthread_create(&tid, NULL, SendMessage, (void *)&data);
        }
    }
}

int main(int argc, char *argv[])
{   
    //Server *server = new Server();  

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

    pthread_t tid[MAX_CONNS];
    int i = 0;
    cout << "Server Started!" << endl;

    while(true)
    {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                
        if (newsockfd < 0)
            error("ERROR on accept");

        cout << "New connection" << endl;

        pthread_create(&tid[i++], NULL, ReceiveMessage, (void *)&newsockfd);

        if (i >= MAX_CONNS) {
            i = 0;
            while (i < MAX_CONNS)
                waitpid(tid[i++], NULL, 0);
            i = 0;
        }
    }
}