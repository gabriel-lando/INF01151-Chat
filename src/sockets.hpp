#ifndef __SOCKET__
#define __SOCKET__

#include "helper.hpp"

#define PING_TIME 1000000 //100.000 us = 100ms 

class ClientSocket {
public:
    ClientSocket(char *ip, int port);
    ~ClientSocket();
    int GetID();
    bool Connect(bool send_ping = true);
    bool SendPing();
    void Disconnect();
    bool IsConnected();
    int SendData(void *pkt, int size);
    bool ReceivePacket(packet *data, int *bytes_read);
    bool ReceiveData(void *data, int size, int *bytes_read);

private:
    void SendPingThread();
    bool SendPong();

    bool isConnected;
    char* ip;
    int port;
    int sockfd;
};

class ServerSocket {
public:
    ServerSocket(int port);
    ~ServerSocket();
    int GetID();
    int Start();
    bool SendPing(int socketid);
    void Disconnect();
    bool IsConnected();
    void DisconnectClient(int socketid);
    int SendData(void *pkt, int size, int socketid);
    bool ReceivePacket(packet *data, int *bytes_read, int socketid);
    bool ReceiveData(void *data, int size, int *bytes_read, int socketid);
    int WaitNewConnection();

private:
    void SendPingThread();
    bool SendPong(int socketid);

    struct sockaddr_in serv_addr, cli_addr;
    bool isConnected;
    int sockfd;
    int port;
};

#endif