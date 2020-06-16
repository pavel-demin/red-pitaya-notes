#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

int main(int argc, char *argv[])
{
  char *end;
  long number;
  struct timespec t;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 1 || number > 3600)
  {
    fprintf(stderr, "Usage: sleep-rand [1-3600]\n");
    return EXIT_FAILURE;
  }

  clock_gettime(CLOCK_REALTIME, &t);
  srand(t.tv_nsec / 1000);
  t.tv_sec = rand() % number;
  t.tv_nsec = rand() % 1000 * 1000000L;
  nanosleep(&t, NULL);
  return EXIT_SUCCESS;
}
