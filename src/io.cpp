#include "io.hpp"

std::mutex io_mtx;

/**
 * Function used to load messages from a specific group
 * While this function is accessing the binary file with the messages, a mutex locks the section code to avoid another
 * functions acessing this resource
 * 
 * @param group the client group
 * @param qty the number of messages that will be retrieved
 * @param response response
 */
int load_messages(string group, int qty, packet *response)
{
    FILE *in;
    string file = "data/" + group + ".bin";

    io_mtx.lock();
    if (!(in = fopen(file.c_str(), "rb")))
    {
        io_mtx.unlock();
        return 0;
    }

    fseek(in, 0L, SEEK_END);
    int size = ftell(in);

    int seek = qty * sizeof(packet);
    if (size < seek)
    {
        seek = size;
    }

    fseek(in, -seek, SEEK_END);
    fread(response, seek, 1, in);
    fclose(in);

    io_mtx.unlock();
    return (seek / sizeof(packet));
}

/**
 * Function used to save a single message pkt into a binary file from a specific group
 * While this function is accessing the binary file with the messages, a mutex locks the section code to avoid another
 * functions acessing this resource
 * 
 * @param pkt Struct that defines the messages sent by clients
 */

bool save_message(packet pkt)
{
    string file = "data/" + string(pkt.groupname) + ".bin";

    io_mtx.lock();
    FILE *out;
    if (!(out = fopen(file.c_str(), "ab+")))
    {
        system("mkdir -p data");
        if (!(out = fopen(file.c_str(), "ab+")))
        {
            io_mtx.unlock();
            return false;
        }
    }

    fwrite(&pkt, sizeof(packet), 1, out);
    fflush(out);
    fclose(out);

    io_mtx.unlock();
    return true;
}