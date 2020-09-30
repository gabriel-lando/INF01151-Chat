#include "client.hpp"

char GetChar();
void SetCursorPosition(int x, int y);
void SetTerminalSize(int x, int y);
string ReadMessage();
void WriteMessage(string message);
char ProcessChar(char c);
void WriteLine(int x, char c);
void PrintLayout(string username, string group);

std::mutex mtx; //Used to avoid concurrency when write message to console

string username;
int usernameLen = 0;
int linePosition = 0;

// The user should establish the connection to the server with the following parameters:
// <username> <groupname> <server_ip_address> <port>
int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr addr;

    /*string*/ username =  check_name(argv[1]) ? argv[1] : "invalid";
    string group_name = check_name(argv[2]) ? argv[2] : "invalid";
    const char* server_ip = argv[3];
    const char* port = argv[4];

    if(username == "invalid"|| group_name == "invalid")
    {
        error("the username and group name can only contain letters, numbers and dot and must have between 4 and 20 characteres");
    }

    usernameLen = username.length();

    char buffer[sizeof(packet)];
    if (argc < 3) 
    {
       error("Please provide more arguments");
    }

    portno = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }

    // Convert IPv4 and IPv6 addresses from text to binary form 
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)  
    { 
        error("Invalid address/ Address not supported"); 
        return -1; 
    } 

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
    {
        error("ERROR connecting");
    }

    PrintLayout(username, group_name);

    while(true)
    {
        /*printf("Please enter the message: ");
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);*/

        string message = ReadMessage();
        packet pkt;
        pkt.timestamp = get_time();
        strcpy(pkt.username, username.c_str());
        strcpy(pkt.message, message.c_str());

        n = write(sockfd, reinterpret_cast<char*>(&pkt), sizeof(packet));
        
        if (n < 0)
        {
            error("ERROR writing to socket");
        }
        
        bzero(buffer, sizeof(packet));
        n = read(sockfd, buffer, sizeof(packet));
        
        if (n < 0)
        {
            error("ERROR reading from socket");
        }

        packet *resp = (packet*)buffer;
        string response = get_timestamp(resp->timestamp) + " " + string(resp->username) + ": " + string(resp->message);

        WriteMessage(response);
    }
}

string ReadMessage(){
    int msg_len = SCREEN_SIZE_Y - (usernameLen + 2) - 1;
    char *temp = (char*) calloc(msg_len * sizeof(char), sizeof(char));
    int count = 0;
    char c;
    while((c = GetChar()) != '\n' && count < msg_len || count == 0){
        c = ProcessChar(c);
        if (c == -1)
            count--;
        else if (c != 0)
            temp[count++] = c;
    }
    temp[count] = '\0';

    return string(temp);
}

void WriteMessage(string message){
    static int curs_pos = SCREEN_SIZE_X - 1;
    static int nameLen = usernameLen + 2;
    
    mtx.lock();

    SetCursorPosition(0, linePosition++);
    cout << message;

    if (linePosition >= SCREEN_SIZE_X - 1){
        SetCursorPosition(0, SCREEN_SIZE_X - 1);
        WriteLine(SCREEN_SIZE_Y - nameLen, ' ');
        SetCursorPosition(0, curs_pos);
        WriteLine(SCREEN_SIZE_Y - nameLen, ' ');
        curs_pos++;
        linePosition--;

        SetCursorPosition(0, curs_pos);
        cout << "\n" << username << ": ";
    }

    SetCursorPosition(nameLen, curs_pos);
    WriteLine(SCREEN_SIZE_Y - nameLen, ' ');
    SetCursorPosition(nameLen, curs_pos);

    mtx.unlock();
}

void PrintLayout(string username, string group){
    mtx.lock();

    SetTerminalSize(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    system("clear");
    SetCursorPosition(0, 0);

    cout << "GRUPO: " << group << endl;
    WriteLine(SCREEN_SIZE_Y, '-');

    SetCursorPosition(0, SCREEN_SIZE_X - 1);
    cout << username << ": ";

    linePosition = 2;
    mtx.unlock();
}

void SetCursorPosition(int x, int y) {
    printf("\033[%d;%dH",y+1,x+1);
}

void SetTerminalSize(int x, int y) {
    printf("\e[8;%d;%dt",x,y);
}

void WriteLine(int x, char c){
    for (int i = 0; i < x; i++)
        putchar(c);
}

char GetChar(void)
{
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

char ProcessChar(char c){
    mtx.lock();

    if (c == 127) {
        putchar('\b');
        putchar(' ');
        putchar('\b');
        c = -1;
    }
    else if (c >= 32 && c <= 126) {
        putchar(c);
    }
    else {
        c = 0;
    }

    mtx.unlock();
    return c;
}