#include "client.hpp"


// The user should establish the connection to the server with the following parameters:
// <username> <groupname> <server_ip_address> <port>
int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr addr;

    string username = argv[1];
    string group_name = argv[2];
    const char* server_ip = argv[3];
    const char* port = argv[4];

    char buffer[256];
    if (argc < 3) 
    {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
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

    printf("Please enter the message: ");
    bzero(buffer, 256);
    fgets(buffer, 255, stdin);
    n = write(sockfd, buffer, strlen(buffer));
    
    if (n < 0)
    {
        error("ERROR writing to socket");
    }

    bzero(buffer, 256);
    n = read(sockfd, buffer, 255);
    
    if (n < 0)
    {
        error("ERROR reading from socket");
    }
    printf("%s\n", buffer);
    return 0;
}