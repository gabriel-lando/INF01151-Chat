#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/ioctl.h>
#include <unistd.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

/*
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
*/
void handle_communication (int sock)
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
   n = write(sock, "I got your message", 18);

   if (n < 0) 
   {
       error("ERROR writing to socket");
   }
}

int main(int argc, char *argv[])
{     
    int sockfd, newsockfd, portno, cli_len;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2) 
    {
       fprintf(stderr,"ERROR, no port provided\n");
       exit(1);
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
    while(1)
    {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                 
        if (newsockfd < 0)
        {
            error("ERROR on accept");
        }
        int pid = fork();

        if (pid < 0)
        {
            error("ERROR on fork");
        }

        if (pid == 0)  
        {
            close(sockfd);
            handle_communication(newsockfd);
            exit(0); 
        }
        else close(newsockfd);
    }
    
    return 0;
    // system call causes the process to block until a client connects to the server
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));

    if (newsockfd < 0)
    {
       error("ERROR on accept");
    }

    bzero(buffer, 256);
    n = read(newsockfd, buffer, 255);

    if (n < 0)
    { 
        error("ERROR reading from socket");
    }

    printf("Here is the message: %s\n",buffer);
    n = write(newsockfd, "I got your message", 18);

    if (n < 0)
    {
        error("ERROR writing to socket");
    }
    return 0;
}