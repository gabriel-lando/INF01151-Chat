#include "helper.hpp"
#include "sockets.hpp"

#define SCREEN_SIZE_X 24
#define SCREEN_SIZE_Y 80

char get_char();
void set_cursor_position(int x, int y);
void set_terminal_size(int x, int y);
string read_message();
void process_packet(packet pkt);
void write_message(string message);
char process_char(unsigned char c);
void write_line(int x, char c);
void print_layout();
void send_message(string message);