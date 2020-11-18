#include "sockets.hpp"

ServerSocket::ServerSocket(int port){
    this->port = port;
    this->isConnected = false;
    this->sockfd = -1;
}

ServerSocket::~ServerSocket() {
    isConnected = false;
    if (sockfd != -1)
        close(sockfd);
}

int ServerSocket::GetID(){
    return sockfd;
}

bool ServerSocket::IsConnected(){
    return isConnected;
}

bool ServerSocket::SendPong(int socketid){
    packet data;
    data.type = PktType::PONG;
    int n = SendData((void*)&data, sizeof(packet), socketid);

    return (n == sizeof(packet));
}

bool ServerSocket::SendPing(int socketid){
    packet data;
    data.type = PktType::PING;
    int n = SendData((void*)&data, sizeof(packet), socketid);

    return (n == sizeof(packet));
}

/*void ServerSocket::SendPingThread(){
    while(isConnected) {
        usleep(PING_TIME);
        if (sockfd <= 0 || !SendPing())
            isConnected = false;
    }
}*/

int ServerSocket::Start(){

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(this->port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        return -1;

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        return -2;

    if (listen(sockfd, MAX_CONNS) != 0)
        return -3;

    return 0;
}

int ServerSocket::WaitNewConnection(){

    int cli_len = sizeof(cli_addr);
    return accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)(&cli_len));
}

void ServerSocket::Disconnect() {
    isConnected = false;
    close(sockfd);
}

void ServerSocket::DisconnectClient(int socketid) {
    close(socketid);
}

int ServerSocket::SendData(void *pkt, int size, int socketid) {
    int n = write(socketid, pkt, size);
    return n;
}

bool ServerSocket::ReceivePacket(packet *data, int *bytes_read, int socketid) {
    /*if (!IsConnected()){
        *bytes_read = 0;
        return false;
    }*/

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

    return true;
}

bool ServerSocket::ReceiveData(void *data, int size, int *bytes_read, int socketid) {
    bzero(data, size);

    *bytes_read = 0;
    *bytes_read = recvfrom(socketid, data, size, 0, NULL, NULL);

    if (*bytes_read != size)
        return false;
    return true;
}