#include "sockets.hpp"

ServerSocket::ServerSocket(int port){
    this->port = port;
    this->isConnected = false;
    this->sockfd = -1;
}

ServerSocket::~ServerSocket() {
    try {
        if (isConnected) {
            isConnected = false;
            if (sockfd != -1)
                close(sockfd);
        }
    } catch(...) { }
}

int ServerSocket::GetID(){
    return sockfd;
}

bool ServerSocket::IsConnected(){
    return isConnected;
}

bool ServerSocket::SendPong(int socketid){
    try {
        packet data;
        data.type = PktType::PONG;
        int n = SendData((void*)&data, sizeof(packet), socketid);
        return (n == sizeof(packet));
    }
    catch(...) {
        return false;
    }
}

bool ServerSocket::SendPing(int socketid){
    try {
        packet data;
        data.type = PktType::PING;
        int n = SendData((void*)&data, sizeof(packet), socketid);
        return (n == sizeof(packet));
    }
    catch(...) {
        return false;
    }
}

/*void ServerSocket::SendPingThread(){
    while(isConnected) {
        usleep(PING_TIME);
        if (sockfd <= 0 || !SendPing())
            isConnected = false;
    }
}*/

int ServerSocket::Start(){
    try {
        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(this->port);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        /*int set = 1;
        setsockopt(sd, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));*/
        if (sockfd < 0)
            return -1;

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
            return -2;

        if (listen(sockfd, MAX_CONNS) != 0)
            return -3;
    } catch(...) {
        return -1;
    }

    return 0;
}

int ServerSocket::WaitNewConnection(){
    try {
        int cli_len = sizeof(cli_addr);
        return accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)(&cli_len));
    } catch(...) {
        return -1;
    }
}

void ServerSocket::Disconnect() {
    try {
        if (isConnected) {
            isConnected = false;
            close(sockfd);
        }
    } catch(...) { }
}

void ServerSocket::DisconnectClient(int socketid) {
    try {
        if (isConnected) {
            isConnected = false;
            close(socketid);
        }
    } catch(...) { }
}

int ServerSocket::SendData(void *pkt, int size, int socketid) {
    try {
        int n = write(socketid, pkt, size);
        return n;
    } catch(...) {
        return 0;
    }
}

bool ServerSocket::ReceivePacket(packet *data, int *bytes_read, int socketid) {
    /*if (!IsConnected()){
        *bytes_read = 0;
        return false;
    }*/

    try {

        bzero(data, sizeof(packet));
        
        *bytes_read = read(socketid, data, sizeof(packet));
        //*bytes_read = recvfrom(socketid, data, sizeof(packet), 0, NULL, NULL);

        if (*bytes_read != sizeof(packet)){
            //Disconnect();
            return false;
        }

        if(data->type == PktType::PING){
            SendPong(socketid);
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

bool ServerSocket::ReceiveData(void *data, int size, int *bytes_read, int socketid) {
    try {
        bzero(data, size);

        *bytes_read = 0;
        *bytes_read = recvfrom(socketid, data, size, 0, NULL, NULL);

        if (*bytes_read != size)
            return false;
        return true;
    } catch(...) {
        return false;
    }
}