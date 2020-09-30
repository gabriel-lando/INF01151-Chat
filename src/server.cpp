#include "server.hpp"

pthread_mutex_t mtxlock = PTHREAD_MUTEX_INITIALIZER;
/*
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
*/
void Server::handle_communication (int sock)
{
    int n;
    char buffer[sizeof(packet)];

    /*recv(sock, buffer, sizeof(packet), 0);

    //sleep(1);
    send(sock, buffer, sizeof(packet), 0);
    close(sock);*/

      
    bzero(buffer, sizeof(packet));
    n = read(sock, buffer, sizeof(packet));

    if (n < 0) 
        error("ERROR reading from socket");

    packet *pkt = (packet*)buffer;
        
    printf("Here is the message: %s\n", pkt->message);
    n = write(sock, buffer, sizeof(packet));

    if (n < 0)
        error("ERROR writing to socket");

    close(sock);
}

void * receive(void * socket) {
    int socket_fd, response, n;
    char buffer[sizeof(packet)];
    socket_fd = *(int *) socket;

    while(true) {
        bzero(buffer, sizeof(packet));
        n = read(socket_fd, buffer, sizeof(packet));

        if (n < 0) 
            error("ERROR reading from socket");

        packet *pkt = (packet*)buffer;

        printf("Here is the message: %s\n", pkt->message);
        n = write(socket_fd, buffer, sizeof(packet));

        if (n < 0)
            error("ERROR writing to socket");
    }

    // Print received message
    /*while(true) {
        response = recvfrom(socket_fd, message, MESSAGE_BUFFER, 0, NULL, NULL);
        if (response) {
            printf("%s", message);
        }
    }*/
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
    int sockfd, newsockfd, portno, cli_len;

    /*
        serv_addr: address of the server
        cli_addr: address of the client connected
    */
    struct sockaddr_in serv_addr, cli_addr;

    // n contains number of characteres read or written
    int n;

    if (argc < 2)
        error("No port provided");

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
       error("ERROR opening socket");

    // sets all values in a buffer to zero.
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    if (listen(sockfd, MAX_THREADS) != 0)
        error("ERROR on listening");

    //pid_t pid[MAX_THREADS];
    pthread_t tid[MAX_THREADS];
    int i = 0;
    cout << "Server Started!" << endl;

    while(true)
    {
        cli_len = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t*)(&cli_len));
                
        if (newsockfd < 0)
            error("ERROR on accept");

        pthread_create(&tid[i++], NULL, receive, (void *)&newsockfd);

        if (i >= MAX_THREADS) {
            i = 0;
            while (i < MAX_THREADS)
                waitpid(tid[i++], NULL, 0);
            i = 0;
        }


        /*int pid_c = fork();

        if (pid_c == 0){
            server->handle_communication(newsockfd);
        }
        else {
            pid[i++] = pid_c;
            if (i > MAX_THREADS) {
                i = 0;
                while (i < MAX_THREADS)
                    waitpid(pid[i++], NULL, 0);
                i = 0;
            }
        }*/

        /*if (pid < 0)
        { 
            error("ERROR on fork");
        }

        if (pid == 0)  
        {
            close(sockfd);
            server->handle_communication(newsockfd);
            exit(0); 
        }
        else close(newsockfd);*/
    }
}