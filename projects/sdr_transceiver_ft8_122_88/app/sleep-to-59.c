#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
  struct timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
  t.tv_sec = 58 - t.tv_sec % 60;
  t.tv_nsec = 999999999L - t.tv_nsec;
  nanosleep(&t, NULL);
  return EXIT_SUCCESS;
}
