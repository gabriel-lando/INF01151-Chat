#ifndef __SOCKET__
#define __SOCKET__

#include "helper.hpp"

#define PING_TIME 100000 //100.000 us = 100ms 

class ClientSocket {
public:
    ClientSocket(char *ip, int port);
    ~ClientSocket();
    int GetID();
    bool Connect();
    bool SendPing();
    void Disconnect();
    bool IsConnected();
    int SendData(void *pkt, int size);
    bool ReceivePacket(packet *data, int *size);
    bool ReceiveData(void *data, int size, int *bytes_read);

private:
    void SendPingThread();
    bool SendPong();

    bool isConnected;
    char* ip;
    int port;
    int sockfd;
};

#endif