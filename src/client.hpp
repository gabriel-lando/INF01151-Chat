#include "helper.hpp"

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