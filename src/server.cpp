#include "server.hpp"

std::mutex mtx;

string groups[MAX_CONNS];

// typedef struct _sockets_id {
//     int id;
//     time_t timestamp;
//     struct _sockets_id * volatile next;
// } sockets_id;

// typedef struct _groups {
//     string groupname;
//     sockets_id * volatile id;
//     struct _groups * volatile next;
// } groups;

// groups *volatile listGroups = NULL;

// sockets_id * GetIDs(string groupname){
//     if (listGroups == NULL)
//         return NULL;

//     std::cerr << "GET IDs from group: " << groupname << endl;

//     groups *tmp = listGroups;

//     while (tmp != NULL){
//         if (tmp->groupname == groupname){

//             /*sockets_id *ids = tmp->id;
//             while (ids != NULL){
//                 std::thread::id this_id = std::this_thread::get_id();
//                 std::cerr << "Thread: " << this_id << endl;
//                 std::cerr << "GET IDs: " << ids->id << endl;
//                 sleep(10);
//                 ids = ids->next;
//             }*/

//             return tmp->id;
//         }
//         tmp = tmp->next;
//     }
//     return NULL;
// }

// void PrintList(){
//     int x = 0, y;

//     std::cerr << "------- Printing List ------" << endl;
//     groups *tmp = listGroups;
//     while(tmp != NULL) {
//         std::cerr << x++ << ": " << tmp->groupname << endl;
//         y = 0;
//         sockets_id *tmp_id = tmp->id;
//         while (tmp_id != NULL){
//             std::cerr << '\t' << y++ << ": " << tmp_id->id << endl;
//             //sleep(10);
//             tmp_id = tmp_id->next;
//         }
//         tmp = tmp->next;
//     }
//     std::cerr << "----------------------------" << endl;
// }

// groups *InsertSocketID(groups *group, int id) {
//     sockets_id *curr = group->id, *last;

//     while (curr != NULL){
//         if (curr->id == id)
//             break;
//         last = curr;
//         curr = curr->next;
//     }

//     if (curr == NULL) {
//         curr = (sockets_id*)malloc(sizeof(sockets_id));
//         curr->id = id;

//         if (group->id == NULL) { group->id = curr; }
//         else { last->next = curr; }
//     }

//     std::cerr << "Insert ID: " << id << " ID: " << curr->id << endl;
//     return group;
// }

// void RemoveIdFromGroup(string groupname, int socket_id) {
//     if (listGroups == NULL)
//         return;
    
//     groups *prev = listGroups, *curr = listGroups;

//     std::cerr << "Remove Group: " << groupname << " ID: " << socket_id << endl;

//     while (curr != NULL) {
//         if (curr->groupname == groupname){
//             sockets_id *prev_id = curr->id, *curr_id = curr->id;
//             while (curr_id != NULL) {
//                 if (curr_id->id == socket_id) {
//                     curr_id->id = 0;
//                     /*if (prev_id == curr_id) { curr_id->id = 0; }
//                     else { prev_id->next = curr_id->next; }
//                     free(curr_id);*/
//                     break;
//                 }

//                 prev_id = curr_id;
//                 curr_id = curr_id->next;
//             }

//             if (curr->id == NULL) {
//                 curr->groupname.clear();
//                 /*if (curr == prev) {
//                     listGroups->groupname = "";
//                 }
//                 else {
//                     prev->next = curr->next;
//                     free(curr);
//                 }*/
//                 /*if (curr == prev) { listGroups = listGroups->next; }
//                 else { prev->next = curr->next; }*/
                
//                 break;
//             }
//         }

//         prev = curr;
//         curr = curr->next;
//     }
// }

// void InsertGroup(string groupname, int socket_id){
//     groups *curr = listGroups, *last;

//     while (curr != NULL){
//         if (curr->groupname == groupname){
//             std::cerr << "Find group: " << curr->groupname << endl;
//             curr = InsertSocketID(curr, socket_id);
//             std::cerr << "Inserted ID: " << socket_id << endl;
//             break;
//         }
//         last = curr;
//         curr = curr->next;
//     }

//     if (curr == NULL) {
//         curr = (groups*)malloc(sizeof(groups));
//         curr->next = NULL;
//         curr->id = NULL;
//         curr->groupname = groupname;
//         curr = InsertSocketID(curr, socket_id);

//         if (listGroups == NULL) { listGroups = curr; }
//         else { last->next = curr; }
//     }

//     std::cerr << "Group: " << curr->groupname << " ID: " << curr->id->id << endl;
// }

// void RemoveID(int id) {
//     if (listGroups == NULL)
//         return;

//     std::cerr << "Removing only ID: " << id << endl;

//     groups *tmp = listGroups;
//     while (tmp != NULL){
//         RemoveIdFromGroup(tmp->groupname, id);
//         tmp = tmp->next;
//     }
// }

// bool ClientIsConnected(int id) {
//     if (listGroups == NULL)
//         return false;
    
//     groups *tmp = listGroups;
//     while (tmp != NULL){
//         sockets_id *ids = GetIDs(tmp->groupname);
//         while (ids != NULL){
//             if (ids->id == id)
//                 return true;
//             ids = ids->next;
//         }
//         tmp = tmp->next;
//     }

//     return false;
// }


// /*
//  There is a separate instance of this function 
//  for each connection.  It handles all communication
//  once a connnection has been established.
// */
// /*void Server::handle_communication (int sock)
// {
//     int n;
//     char buffer[sizeof(packet)];
      
//     bzero(buffer, sizeof(packet));
//     n = read(sock, buffer, sizeof(packet));

//     if (n < 0) 
//         error("ERROR reading from socket");

//     packet *pkt = (packet*)buffer;
        
//     printf("Here is the message: %s\n", pkt->message);
//     n = write(sock, buffer, sizeof(packet));

//     if (n < 0)
//         error("ERROR writing to socket");

//     close(sock);
// }*/

void SendMessage(packet content) {
    for (int i = 0; i < MAX_CONNS; i++) {
        if (groups[i] != content.groupname)
            continue;

        mtx.lock();
        int n = write(i, reinterpret_cast<char*>(&content), sizeof(packet));
        mtx.unlock();

        if (n <= 0)
            groups[i].clear();
    }
}

void ReceiveMessage(int socket_fd) {
    while (true) {
        packet data, buffer;
        bzero(&buffer, sizeof(packet));

        int n = read(socket_fd, &buffer, sizeof(packet));
        data = buffer;

        if (n != sizeof(packet)) {
            groups[socket_fd].clear();
            break;
        }

        if (strlen(data.groupname)) {
            groups[socket_fd] = string(data.groupname);
            printf("%s %s [%s]: %s\n", (get_timestamp(data.timestamp)).c_str(), data.username, data.groupname, data.message);

            std::thread write_message (SendMessage, data);
            write_message.detach();
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

    int i = 0;
    std::cerr << "Server Started!" << endl;

    std::thread tid[MAX_CONNS];

    while(true)
    {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                
        if (newsockfd < 0)
            error("ERROR on accept");

        std::cerr << "New connection" << endl;

        tid[i++] = std::thread(ReceiveMessage, newsockfd);

        if (i >= MAX_CONNS) {
            i = 0;
            while (i < MAX_CONNS)
                tid[i++].join();
            i = 0;
        }
    }
}