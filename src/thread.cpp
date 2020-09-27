#include "thread.hpp"

Thread::~Thread() {};

void Thread::start(void * thread_function)
{
    // TODO: investigate better way to receive and pass function pointer :(
    pthread_create(&id, NULL, (void* (*)(void *))thread_function, this);
}
