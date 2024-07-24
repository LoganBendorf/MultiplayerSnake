
#ifndef timeHelpers
#define timeHelpers

#include <time.h>

//static unsigned int next_random = 1;

unsigned int my_rand(void);
void my_srand(unsigned int seed);

long long diffMilli(struct timespec *start, struct timespec *end);

#endif