
#include <time.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __linux__
#endif

static unsigned int next_random = 1;

unsigned int my_rand(void)   // RAND_MAX assumed to be 32767
{
    next_random = next_random * 1103515245 + 12345;
    return (unsigned int)(next_random/65536) % 32768;
}

void my_srand(unsigned int seed) {
    next_random = seed;
}


long long diffMilli(struct timespec *start, struct timespec *end) {
    return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
        ((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}