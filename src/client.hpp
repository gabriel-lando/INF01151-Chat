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