#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  srand(t.tv_nsec / 1000);
  t.tv_sec = rand() % 300;
  t.tv_nsec = rand() % 1000 * 1000000L;
  nanosleep(&t, NULL);
  return EXIT_SUCCESS;
}
