
#include <time.h>
#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef __linux__
#endif


long long diffMilli(struct timespec *start, struct timespec *end) {
    return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
        ((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}