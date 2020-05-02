#include "global.h"

pthread_mutex_t gCountMutex;
int gPort = -1;
long long gCount = 0;
struct timeval gTime = { 0, 0 };
long long gMaxCount = 0;