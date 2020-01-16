#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
  int fd, i;
  char *end;
  volatile void *cfg;
  long number[3];

  for(i = 0; i < 3; ++i)
  {
    errno = 0;
    number[i] = (argc == 4) ? strtol(argv[i + 1], &end, 10) : -1;
    if(errno != 0 || end == argv[i + 1])
    {
      printf("Usage: gen [1-2] [0-32766] [0-61440000]\n");
      return EXIT_FAILURE;
    }
  }

  if(number[0] < 1 || number[0] > 2 || number[1] < 0 || number[1] > 32766 || number[2] < 0 || number[2] > 61440000)
  {
    printf("Usage: gen [1-2] [0-32766] [0-61440000]\n");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);

  switch(number[0])
  {
    case 1:
      *(uint32_t *)(cfg + 4) = (uint32_t)floor(number[2] / 125.0e6 * (1<<30) + 0.5);
      *(uint16_t *)(cfg + 12) = (uint16_t)number[1];
      break;
    case 2:
      *(uint32_t *)(cfg + 8) = (uint32_t)floor(number[2] / 125.0e6 * (1<<30) + 0.5);
      *(uint16_t *)(cfg + 14) = (uint16_t)number[1];
      break;
  }

  return EXIT_SUCCESS;
}
