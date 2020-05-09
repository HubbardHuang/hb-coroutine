#include "global.h"

pthread_mutex_t gCountMutex;
int gPort = -1;
long long gCount = 0;
struct timeval gTime = { 0, 0 };
long long gMaxCount = 0;
long long gClientConnectionCount;
long long gServerConnectionCount;
int gFdType[1000] = { OTHER };

std::list<std::vector<long long>> gContent;

FILE* gDataTxt = nullptr;