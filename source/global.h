#ifndef GLOBAL_H
#define GLOBAL_H

#include <list>
#include <vector>

#include <pthread.h>
#include <sys/time.h>

extern pthread_mutex_t gCountMutex;
extern int gPort;
extern long long gCount;
extern struct timeval gTime;
extern long long gMaxCount;
extern long long gClientConnectionCount;
extern long long gServerConnectionCount;

#define ClientConnection 1
#define ServerConnection 2
#define OTHER 3
extern int gFdType[1000];

#define LINE_MAX 5
extern std::list<std::vector<long long>> gContent;

#endif