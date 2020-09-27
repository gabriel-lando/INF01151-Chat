#include "server.hpp"

/*
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
*/
void Server::handle_communication (int sock)
{
    int n;
    char buffer[256];

      
    bzero(buffer, 256);
    n = read(sock, buffer, 255);

    if (n < 0) 
    {
        error("ERROR reading from socket");
    }
        
    printf("Here is the message: %s\n", buffer);
    n = write(sock, "I got your message", 255);

    if (n < 0) 
    {
        error("ERROR writing to socket");
    }
}

int main(int argc, char *argv[])
{   
    Server *server = new Server();  

    /*
        sockfd: file descriptor, will store values returned by socket system call
        newsockfd: file descriptor, will store values returned by accept system call
        portno: port number on which the server accepts connections
        cli_len: size of the address of the client 
    */
    int sockfd, newsockfd, portno, cli_len, pid;

    /*
        serv_addr: address of the server
        cli_addr: address of the client connected
    */
    struct sockaddr_in serv_addr, cli_addr;

    // n contains number of characteres read or written
    int n;

    if (argc < 2) 
    {
        error("No port provided");
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    { 
       error("ERROR opening socket");
    }

    // sets all values in a buffer to zero.
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    { 
        error("ERROR on binding");
    }

    listen(sockfd, 256);
    cli_len = sizeof(cli_addr);

    cout << "Server Started!" << endl;

    while(true)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }

        pid = fork();

        if (pid < 0)
        { 
            error("ERROR on fork");
        }

        if (pid == 0)  
        {
            close(sockfd);
            server->handle_communication(newsockfd);
            exit(0); 
        }
        else close(newsockfd);
    }
}
