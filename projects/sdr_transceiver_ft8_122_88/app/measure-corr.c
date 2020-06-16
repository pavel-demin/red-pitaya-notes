#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
  int fd, i;
  char *end;
  volatile void *cfg, *sts;
  volatile uint32_t *fifo;
  volatile uint8_t *rst;
  volatile uint16_t *cntr;
  long number;
  uint32_t buffer;
  float corr;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 1 || number > 30)
  {
    fprintf(stderr, "Usage: measure-corr [1-30]\n");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  fifo = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40013000);

  rst = (uint8_t *)(cfg + 3);
  cntr = (uint16_t *)(sts + 28);

  *rst &= ~1;
  *rst |= 1;

  sleep(number + 1);

  if(*cntr < number)
  {
    fprintf(stderr, "Not enough PPS pulses.\n");
    return EXIT_FAILURE;
  }

  buffer = 0;

  for(i = 0; i < number; ++i)
  {
    buffer += *fifo + 1;
  }

  corr = (122.88e6 * number / buffer - 1.0) * 1.0e6;

  if(corr < -100.0 || corr > 100.0)
  {
    fprintf(stderr, "Correction value is out of range.\n");
    return EXIT_FAILURE;
  }

  printf("%.2f\n", corr);

  return EXIT_SUCCESS;
}
