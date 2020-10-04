#include "helper.hpp"

#define SCREEN_SIZE_X 20
#define SCREEN_SIZE_Y 50

class Client
{
    public:
    Client(string username, string group_name, string server_ip, string port);
    ~Client();

    string username;
    string group_name;
    const char* server_ip;
    int port;

    private:
};

char GetChar();
void SetCursorPosition(int x, int y);
void SetTerminalSize(int x, int y);
string ReadMessage();
void ProcessPacket(packet pkt);
void WriteMessage(string message);
char ProcessChar(unsigned char c);
void WriteLine(int x, char c);
void PrintLayout(string username, string group);
void SendMessage(string message, string user, string group, int socket_id);