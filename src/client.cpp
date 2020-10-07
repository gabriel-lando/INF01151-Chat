#include "client.hpp"

std::mutex mtx; //Used to avoid concurrency when write message to console

string username;
struct termios currt;
int usernameLen = 0;
int linePosition = 0;
volatile int message_len = 0;
volatile char current_msg[SCREEN_SIZE_Y] = { '\0' };
volatile 

int curs_initial_pos = SCREEN_SIZE_X - 1;

typedef struct {
    char* prompt;
    int socket;
} thread_data;

void error_and_reset(string msg)
{
    cerr << msg << endl;
    tcsetattr(STDIN_FILENO, TCSANOW, &currt);
    exit(0);
}

void ReceiveMessage(int socket_fd) {
    packet buffer;

    while(true) {
        bzero(&buffer, sizeof(packet));
        int response = recvfrom(socket_fd, &buffer, sizeof(packet), 0, NULL, NULL);
  
        if (response == -1)
            error_and_reset("Recv failed\n");
        else if (response == 0)
            error_and_reset("Peer disconnected\n");
        
        if (response == sizeof(packet))
            ProcessPacket(buffer);
        else
            error_and_reset("Connection Timeout.");
    }
}

// The user should establish the connection to the server with the following parameters:
// <username> <groupname> <server_ip_address> <port>
int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr addr;


    if (argc != 5)
        error("Use: " + string(argv[0]) + " <username> <groupname> <server_ip> <server_port>");

    username =  check_name(argv[1]) ? argv[1] : "invalid";
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

    std::thread receive_thread (ReceiveMessage, sockfd);
    receive_thread.detach();

    SendMessage("", username, group_name, sockfd);

    tcgetattr(STDIN_FILENO, &currt);
    while(true) {
        string message = ReadMessage();
        SendMessage(message, username, group_name, sockfd);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &currt);
}

void SendMessage(string message, string user, string group, int socket_id){
    packet pkt;
    pkt.timestamp = get_time();
    strcpy(pkt.groupname, group.c_str());
    strcpy(pkt.username, user.c_str());
    strcpy(pkt.message, message.c_str());

    int n = write(socket_id, reinterpret_cast<char*>(&pkt), sizeof(packet));
    if (n < 0)
        error_and_reset("ERROR writing to socket");
}

string ReadMessage() {

    mtx.lock();
    SetCursorPosition(usernameLen + 2, curs_initial_pos);
    WriteLine(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
    SetCursorPosition(usernameLen + 2, curs_initial_pos);
    mtx.unlock();

    int msg_len = SCREEN_SIZE_Y - (usernameLen + 2) - 1 - 9;
    
    message_len = 0;
    unsigned char c, prev_c = 0;

    current_msg[0] = '\0';

    while((c = GetChar()) != '\n' && message_len < msg_len || message_len == 0){
        c = ProcessChar(c);
        if (c > 0) {
            current_msg[message_len++] = c;
            current_msg[message_len] = '\0';
        }
    }
    current_msg[message_len] = '\0';

    return string((char*)current_msg);
}

void ProcessPacket(packet pkt) {
    string msguser = string(pkt.username);
    if (msguser == username)
        msguser = "voce";
    
    string message = get_timestamp(pkt.timestamp) + " " + msguser + ": " + string(pkt.message);

    WriteMessage(message);
}

void WriteMessage(string message){
    mtx.lock();

    SetCursorPosition(0, linePosition++);
    cerr << message;

    if (message.length() > SCREEN_SIZE_Y)
        linePosition++;

    int cur_pos = usernameLen + 2 + message_len;
    if (linePosition >= SCREEN_SIZE_X - 1){
        SetCursorPosition(0, SCREEN_SIZE_X - 1);
        WriteLine(SCREEN_SIZE_Y - cur_pos, ' ');
        SetCursorPosition(0, curs_initial_pos);
        WriteLine(SCREEN_SIZE_Y - cur_pos, ' ');
        curs_initial_pos++;
        linePosition--;

        SetCursorPosition(0, curs_initial_pos);
        fprintf(stderr, "\n%s: %s", username.c_str(), current_msg);
    }

    SetCursorPosition(cur_pos, curs_initial_pos);
    WriteLine(SCREEN_SIZE_Y - cur_pos, ' ');
    SetCursorPosition(cur_pos, curs_initial_pos);

    mtx.unlock();
}

void PrintLayout(string username, string group){
    mtx.lock();

    SetTerminalSize(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    system("clear");
    SetCursorPosition(0, 0);

    fprintf(stderr, "GRUPO: %s\n", group.c_str());
    WriteLine(SCREEN_SIZE_Y, '-');

    SetCursorPosition(0, SCREEN_SIZE_X - 1);
    fprintf(stderr, "%s: ", username.c_str());

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

char GetChar() {
    int ch;
    struct termios oldt;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    currt = oldt;
    newt = oldt; 
    newt.c_lflag &= ~(ICANON | ECHO); 
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

char ProcessChar(unsigned char c) {
    
    if (c >= 32 && c < 127 /*255 && c != 127*/) {
        mtx.lock();
        cerr << c;
        mtx.unlock();

        return c;
    }

    if (c == 127 && message_len > 0) {
        mtx.lock();
        cerr << "\b \b"; // backspace space backspace
        mtx.unlock();
        (message_len)--;
    }
    
    return 0;
}