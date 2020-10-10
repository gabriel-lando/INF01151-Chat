#include "helper.hpp"

void error(string msg)
{
    cerr << msg << endl;
    exit(0);
}

/**
 * Function to validate group name and user name
 * They should have > 4 and < 20 characteres, start with letter and have only letters, numbers and dot (.)
 * @param name string to be validated
 */

bool check_name(const string &name)
{
    if (!((name[0] <= 'z' && name[0] >= 'a') || (name[0] <= 'Z' && name[0] >= 'A')) || name.length() > 20 || name.length() < 4)
        return false;

    for (int i = 0; i < (int)name.size(); i++)
    {
        if (!((name[i] <= 'z' && name[i] >= 'a') || (name[i] <= 'Z' && name[i] >= 'A') || (isdigit(name[i])) || (name[i] == '.')))
            return false;
    }
    return true;
}

time_t get_time()
{
    return std::time(nullptr);
}

string get_timestamp(time_t time)
{
    auto tm = *localtime(&time);
    std::ostringstream oss;
    oss << put_time(&tm, "%H:%M:%S");
    return oss.str();
}
