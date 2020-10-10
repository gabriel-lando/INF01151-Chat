#include "client.hpp"

std::mutex mtx; //Used to avoid concurrency when write message to console

string username;
string group_name;

struct termios currt;
int usernameLen = 0;
int linePosition = 0;
volatile int message_len = 0;
volatile char current_msg[SCREEN_SIZE_Y] = {'\0'};
volatile

    int curs_initial_pos = SCREEN_SIZE_X - 1;

typedef struct
{
    char *prompt;
    int socket;
} thread_data;

/**
 * Funcion used to terminate the application and log the errors
 * @param msg output message
 */

void error_and_reset(string msg)
{
    cerr << msg << endl;
    tcsetattr(STDIN_FILENO, TCSANOW, &currt);
    exit(0);
}

/**
 * Loop started by a thread for the client to receive messages from others 
 * @param socket_fd file descriptor, will store values returned by socket system call
 */

void receive_message(int socket_fd)
{
    packet buffer;

    while (true)
    {
        bzero(&buffer, sizeof(packet));
        int response = recvfrom(socket_fd, &buffer, sizeof(packet), 0, NULL, NULL);

        if (response == -1)
            error_and_reset("Recv failed\n");
        else if (response == 0)
            error_and_reset("Peer disconnected\n");

        /* Success receiving a message */
        if (response == sizeof(packet))
            process_packet(buffer);
        else
            error_and_reset("Connection Timeout.");
    }
}

/**
 * Client main loop
 * 
 * The user should establish the connection to the server with the following parameters:
 * <username> <groupname> <server_ip_address> <port>
 * @param socket_fd file descriptor, will store values returned by socket system call
 */

int main(int argc, char *argv[])
{
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;
    struct in_addr addr;

    if (argc != 5)
        error("Use: " + string(argv[0]) + " <username> <groupname> <server_ip> <server_port>");

    /* username and group_name name validations */
    username = check_name(argv[1]) ? argv[1] : "invalid";
    group_name = check_name(argv[2]) ? argv[2] : "invalid";

    const char *server_ip = argv[3];
    const char *port = argv[4];

    if (username == "invalid" || group_name == "invalid")
        error("the username and group name can only contain letters, numbers and dot and must have between 4 and 20 characteres");

    usernameLen = username.length();

    char buffer[sizeof(packet)];
    if (argc < 3)
        error("Please provide more arguments");

    portno = atoi(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        error("ERROR opening socket");

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
        error("Invalid address/ Address not supported");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    /* Client connected successfully, call print_layout to configure user interface on the terminal */
    print_layout();

    /* Thread to receive messages on that client */
    std::thread receive_thread(receive_message, sockfd);
    receive_thread.detach();

    send_message("", sockfd);

    tcgetattr(STDIN_FILENO, &currt);

    while (true)
    {
        string message = read_message();
        send_message(message, sockfd);
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &currt);
}

/****************************************************************************
 *  Console-related functions
 ***************************************************************************/


/**
 * Function to write the message on the socket_id, shared by client and server
 * 
 * @param message the message that should be written
 * @param socket_id socket identifier
 */
void send_message(string message, int socket_id)
{
    packet pkt;
    pkt.timestamp = get_time();
    strcpy(pkt.groupname, group_name.c_str());
    strcpy(pkt.username, username.c_str());
    strcpy(pkt.message, message.c_str());

    int n = write(socket_id, reinterpret_cast<char *>(&pkt), sizeof(packet));
    if (n < 0)
        error_and_reset("ERROR writing to socket");
}

/**
 * Function used to retrieve the message that the client wrote on the user interface terminal
 */
string read_message()
{
    /* A mutex should be used here to avoid having the cursor setted on a wrong position
       This could happen, for example, if we are setting the cursor to a new position and the client receives a message
     */
    mtx.lock();
    set_cursor_position(usernameLen + 2, curs_initial_pos);
    write_line(SCREEN_SIZE_Y - (usernameLen + 2), ' ');
    set_cursor_position(usernameLen + 2, curs_initial_pos);
    mtx.unlock();

    int msg_len = SCREEN_SIZE_Y - (usernameLen + 2) - 1 - 9;

    message_len = 0;
    unsigned char c, prev_c = 0;

    current_msg[0] = '\0';

    while ((c = get_char()) != '\n' && message_len < msg_len || message_len == 0)
    {
        c = process_char(c);
        if (c > 0)
        {
            current_msg[message_len++] = c;
            current_msg[message_len] = '\0';
        }
    }
    current_msg[message_len] = '\0';

    return string((char *)current_msg);
}


/**
 * Receives the message and formats to display it with the timestamp and the user on console
 * 
 * @param pkt Struct that defines the messages sent by clients
 */
void process_packet(packet pkt)
{
    string msguser = string(pkt.username);
    if (msguser == username)
        msguser = "voce";

    string message = get_timestamp(pkt.timestamp) + " " + msguser + ": " + string(pkt.message);

    write_message(message);
}

/**
 * Receives the message and formats to display it with the timestamp and the user on console
 *  This function should use a mutex to avoid that the cursor goes to a wrong position on the console
 * @param message string message already organized in the right format to be written on console
 */
void write_message(string message)
{
    mtx.lock();

    set_cursor_position(0, linePosition++);
    cerr << message;

    if (message.length() > SCREEN_SIZE_Y)
        linePosition++;

    int cur_pos = usernameLen + 2 + message_len;
    if (linePosition >= SCREEN_SIZE_X - 1)
    {
        set_cursor_position(0, SCREEN_SIZE_X - 1);
        write_line(SCREEN_SIZE_Y - cur_pos, ' ');
        set_cursor_position(0, curs_initial_pos);
        write_line(SCREEN_SIZE_Y - cur_pos, ' ');
        curs_initial_pos++;
        linePosition--;

        set_cursor_position(0, curs_initial_pos);
        fprintf(stderr, "\n%s: %s", username.c_str(), current_msg);
    }

    set_cursor_position(cur_pos, curs_initial_pos);
    write_line(SCREEN_SIZE_Y - cur_pos, ' ');
    set_cursor_position(cur_pos, curs_initial_pos);

    mtx.unlock();
}

/**
 * Function called when the client successfully connects to the server
 * Starts the console interface showing the group and user
 * This function should use a mutex to avoid that the cursor goes to a wrong position on the console
 * @param message string message already organized in the right format to be written on console
 */
void print_layout()
{
    mtx.lock();

    set_terminal_size(SCREEN_SIZE_X, SCREEN_SIZE_Y);
    system("clear");
    set_cursor_position(0, 0);

    fprintf(stderr, "GRUPO: %s\n", group_name.c_str());
    write_line(SCREEN_SIZE_Y, '-');

    set_cursor_position(0, SCREEN_SIZE_X - 1);
    fprintf(stderr, "%s: ", username.c_str());

    linePosition = 2;
    mtx.unlock();
}


void set_cursor_position(int x, int y)
{
    fprintf(stderr, "\033[%d;%dH", y + 1, x + 1);
}


void set_terminal_size(int x, int y)
{
    fprintf(stderr, "\e[8;%d;%dt", x, y);
}

void write_line(int x, char c)
{
    for (int i = 0; i < x; i++)
        cerr << c;
}

char get_char()
{
    int ch;
    struct termios oldt;
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    currt = oldt;
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

char process_char(unsigned char c)
{

    if (c >= 32 && c < 127 /*255 && c != 127*/)
    {
        mtx.lock();
        cerr << c;
        mtx.unlock();

        return c;
    }

    if (c == 127 && message_len > 0)
    {
        mtx.lock();
        cerr << "\b \b"; // backspace space backspace
        mtx.unlock();
        (message_len)--;
    }

    return 0;
}