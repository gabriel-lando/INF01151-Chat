#include "sockets.hpp"

ClientSocket::ClientSocket(char *ip, int port){
    this->ip = (char*) malloc((strlen(ip)+1) * sizeof(char));
    strcpy(this->ip, ip);
    this->port = port;
    this->isConnected = false;
    this->sockfd = -1;
}

ClientSocket::~ClientSocket() {
    isConnected = false;
    if (sockfd != -1)
        close(sockfd);
}

int ClientSocket::GetID(){
    return sockfd;
}

bool ClientSocket::IsConnected(){
    return isConnected;
}

bool ClientSocket::SendPong(){
    packet data;
    data.type = PktType::PONG;
    int n = SendData((void*)&data, sizeof(packet));

    return (n == sizeof(packet));
}

bool ClientSocket::SendPing(){
    packet data;
    data.type = PktType::PING;
    int n = SendData((void*)&data, sizeof(packet));

    return (n == sizeof(packet));
}

void ClientSocket::SendPingThread(){
    while(isConnected) {
        usleep(PING_TIME);
        if (sockfd <= 0 || !SendPing())
            isConnected = false;
    }
}

bool ClientSocket::Connect(){
    struct sockaddr_in sockaddr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0 || inet_pton(AF_INET, ip, &sockaddr.sin_addr) <= 0)
        return false;

    bzero((char *)&sockaddr, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&sockaddr, sizeof(sockaddr)) < 0)
        return false;

    isConnected = true;

    thread checkConnection(&ClientSocket::SendPingThread, this);
    checkConnection.detach();

    return true;
}

void ClientSocket::Disconnect() {
    isConnected = false;
    close(sockfd);
}

int ClientSocket::SendData(void *pkt, int size) {
    int n = write(sockfd, pkt, size);
    return n;
}

bool ClientSocket::ReceivePacket(packet *data, int *size) {

    if (!IsConnected())
        return false;

    packet buffer;
    bzero(&buffer, sizeof(packet));
    
    *size = recvfrom(sockfd, &buffer, sizeof(packet), 0, NULL, NULL);

    if (*size != sizeof(packet))
    {
        Disconnect();
        return false;
    }

    if(buffer.type == PktType::PING){
        SendPong();
        return false;
    }
    else if(buffer.type == PktType::PONG){
        return false;
    }

    *data = buffer;
    return true;
}

bool ClientSocket::ReceiveData(void *data, int size, int *bytes_read) {
    bzero(data, size);

    *bytes_read = 0;
    *bytes_read = recvfrom(sockfd, data, size, 0, NULL, NULL);

    if (*bytes_read != size)
        return false;
    return true;
}