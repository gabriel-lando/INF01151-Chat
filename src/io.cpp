#include "io.hpp"

std::mutex io_mtx;

int LoadMessages(string group, int qty, packet *response)
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
    int read = fread(response, seek, 1, in);
    fclose(in);

    io_mtx.unlock();
    return (seek / sizeof(packet));
}

bool SaveMessage(packet pkt)
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