#include "client.hpp"

std::mutex mtx; //Used to avoid concurrency when write message to console

string username;
int usernameLen = 0;
int linePosition = 0;

int curs_pos = SCREEN_SIZE_X - 1;

typedef struct {
    char* prompt;
    int socket;
} thread_data;


void * receive(void * sockfd) {
    int socket_fd, response;
    char buffer[sizeof(packet)];
    socket_fd = *(int*)sockfd;

    while(true) {
        bzero(buffer, sizeof(packet));
        response = recvfrom(socket_fd, buffer, sizeof(packet), 0, NULL, NULL);

        if (response == -1)
            error("Recv failed\n");
        else if (response == 0)
            error("Peer disconnected\n");

        packet *resp = (packet*)buffer;
        
        ProcessPacket(resp);
    }
}

// The user should establish the connection to the server with the following parameters:
// <username> <groupname> <server_ip_address> <port>
int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    pthread_t receive_thread;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr addr;

    /*string*/ username =  check_name(argv[1]) ? argv[1] : "invalid";
    string group_name = check_name(argv[2]) ? argv[2] : "invalid";
    const char* server_ip = argv[3];
    const char* port = argv[4];

    if(username == "invalid"|| group_name == "invalid")
        error("the username and group name can only contain letters, numbers and dot and must have between 4 and 20 characteres");

    usernameLen = username.length();

    char buffer[sizeof(packet)];
    if (argc < 3)
       error("Please provide more arguments");

    portno = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        error("Invalid address/ Address not supported");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");
    
    PrintLayout(username, group_name);

    pthread_create(&receive_thread, NULL, receive, (void *) &sockfd);

    SendMessage(string("<entrou no grupo>"), username, group_name, sockfd);
    while(true)
    {
        string message = ReadMessage();
        SendMessage(message, username, group_name, sockfd);
    }
}

void SendMessage(string message, string user, string group, int socket_id){
    packet pkt;
    pkt.timestamp = get_time();
    strcpy(pkt.groupname, group.c_str());
    strcpy(pkt.username, user.c_str());
    strcpy(pkt.message, message.c_str());

    int n = write(socket_id, reinterpret_cast<char*>(&pkt), sizeof(packet));
    if (n < 0)
        error("ERROR writing to socket");
}

string ReadMessage() {

    mtx.lock();
    SetCursorPosition(usernameLen + 2, curs_pos);
    WriteLine(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
    SetCursorPosition(usernameLen + 2, curs_pos);
    mtx.unlock();

    int msg_len = SCREEN_SIZE_Y - (usernameLen + 2) - 1;
    char *temp = (char*) calloc(msg_len * sizeof(char), sizeof(char));
    int count = 0;
    char c;
    // ToDo: add a function to set cursor to current position when a message is received
    while((c = GetChar()) != '\n' && count < msg_len || count == 0){
        c = ProcessChar(c, &count);
        if (c > 0)
            temp[count++] = c;
    }
    temp[count] = '\0';

    return string(temp);
}

void ProcessPacket(packet *pkt) {
    string msguser = string(pkt->username);
    if (msguser == username)
        msguser = "voce";
    
    string message = get_timestamp(pkt->timestamp) + " " + msguser + ": " + string(pkt->message);

    WriteMessage(message);
}

void WriteMessage(string message){
    mtx.lock();

    SetCursorPosition(0, linePosition++);
    cerr << message;

    if (linePosition >= SCREEN_SIZE_X - 1){
        SetCursorPosition(0, SCREEN_SIZE_X - 1);
        WriteLine(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
        SetCursorPosition(0, curs_pos);
        WriteLine(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
        curs_pos++;
        linePosition--;

        SetCursorPosition(0, curs_pos);
        cerr << "\n" << username << ": ";
    }

    SetCursorPosition(usernameLen + 2, curs_pos);
    WriteLine(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
    SetCursorPosition(usernameLen + 2, curs_pos);

    mtx.unlock();
}

void PrintLayout(string username, string group){
    mtx.lock();

    SetTerminalSize(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    system("clear");
    SetCursorPosition(0, 0);

    cerr << "GRUPO: " << group << endl;
    WriteLine(SCREEN_SIZE_Y, '-');

    SetCursorPosition(0, SCREEN_SIZE_X - 1);
    cerr << username << ": ";

    linePosition = 2;
    mtx.unlock();
}

void SetCursorPosition(int x, int y) {
    fprintf(stderr, "\033[%d;%dH",y+1,x+1);
}

void SetTerminalSize(int x, int y) {
    fprintf(stderr, "\e[8;%d;%dt",x,y);
}

void WriteLine(int x, char c) {
    for (int i = 0; i < x; i++)
        cerr << c;
}

char GetChar(void) {
    int ch;
    struct termios oldt;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt); 
    newt = oldt; 
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

char ProcessChar(char c, int * count) {
    if (c >= 32 && c <= 126) {
        mtx.lock();
        cerr << c;
        mtx.unlock();

        return c;
    }

    if (c == 127 && *count > 0) {
        mtx.lock();
        cerr << "\b \b"; // backspace space backspace
        mtx.unlock();
        (*count)--;
    }
    
    return 0;
}