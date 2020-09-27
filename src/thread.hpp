#include "helper.hpp"

class Thread
{
    public:       
        void start(void * thread_function);

        pthread_t id;
};