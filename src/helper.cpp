#include "helper.hpp"

void error(char *msg)
{
    perror(msg);
    exit(0);
}