#ifndef GLOBAL_H
#define GLOBAL_H

#include <pthread.h>
#include <sys/time.h>

extern pthread_mutex_t gCountMutex;
extern int gPort;
extern long long gCount;
extern struct timeval gTime;
extern long long gMaxCount;

#endif