#include "sockets.hpp"

ClientSocket::ClientSocket(const char *ip, int port){
    this->ip = (char*) malloc((strlen(ip)+1) * sizeof(char));
    strcpy(this->ip, ip);
    this->port = port;
    this->isConnected = false;
    this->sockfd = -1;
}

ClientSocket::~ClientSocket() {
    try {
        if (isConnected) {
            isConnected = false;
            if (sockfd != -1)
                close(sockfd);
        }
    } catch(...) { }
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

    try {
        int n = SendData((void*)&data, sizeof(packet));
        return (n == sizeof(packet));
    } catch(...) {
        return false;
    }
}

bool ClientSocket::SendPing(){
    packet data;
    data.type = PktType::PING;

    try {
        int n = SendData((void*)&data, sizeof(packet));
        return (n == sizeof(packet));
    } catch(...) {
        return false;
    }
}

void ClientSocket::SendPingThread(){
    while(isConnected) {
        usleep(PING_TIME);
        try {
            if (sockfd <= 0 || !SendPing()) {
                fprintf(stderr, "PING Error\n");
                isConnected = false;
            }
        } catch(...) {
            isConnected = false;
            break;
        }
    }
}

bool ClientSocket::Connect(bool send_ping){
    try {
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

        fprintf(stderr, "Socket ID: %d\n", sockfd);

        if (send_ping) {
            thread checkConnection(&ClientSocket::SendPingThread, this);
            checkConnection.detach();
        }
    } catch(...) {
        isConnected = false;
        return false;
    }

    return true;
}

void ClientSocket::Disconnect() {
    try {
        if (isConnected) {
            isConnected = false;
            close(sockfd);
        }
    } catch(...) { }
}

int ClientSocket::SendData(void *pkt, int size) {
    if (!IsConnected())
        return 0;

    try {
        int n = write(sockfd, pkt, size);
        return n;
    } catch(...) {
        return 0;
    }
}

bool ClientSocket::ReceivePacket(packet *data, int *bytes_read) {
    
    if (!IsConnected()) {
        *bytes_read = 0;
        return false;
    }

    try {
        *bytes_read = 0;
        bzero(data, sizeof(packet));
        *bytes_read = recvfrom(sockfd, data, sizeof(packet), 0, NULL, NULL);      

        if (*bytes_read != sizeof(packet)){
            return false;
        }

        if(data->type == PktType::PING){
            SendPong();
            return false;
        }
        else if(data->type == PktType::PONG){
            return false;
        }
    } catch(...) {
        return false;
    }

    return true;
}

bool ClientSocket::ReceiveData(void *data, int size, int *bytes_read) {
    if (!IsConnected()) {
        *bytes_read = 0;
        return false;
    }

    try {
        *bytes_read = 0;
        bzero(data, size);

        *bytes_read = recvfrom(sockfd, data, size, 0, NULL, NULL);

        if (*bytes_read != size)
            return false;
    } catch(...) {
        return false;
    }

    return true;
}