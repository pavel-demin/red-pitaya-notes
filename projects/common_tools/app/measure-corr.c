#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>

void usage()
{
  fprintf(stderr, "Usage: measure-corr freq time\n");
  fprintf(stderr, " freq - frequency expressed in MHz (122.88 or 125),\n");
  fprintf(stderr, " time - measurement time expressed in seconds (from 1 to 30).\n");
}

int main(int argc, char *argv[])
{
  int fd, i;
  char *end;
  volatile void *cfg, *sts;
  volatile uint32_t *fifo;
  volatile uint8_t *rst;
  volatile uint16_t *cntr;
  double freq;
  long time;
  uint32_t buffer;
  float corr;

  if(argc != 3)
  {
    usage();
    return EXIT_FAILURE;
  }

  errno = 0;
  freq = strtod(argv[1], &end);
  if(errno != 0 || end == argv[1] || (freq != 122.88 && freq != 125.0))
  {
    usage();
    return EXIT_FAILURE;
  }

  errno = 0;
  time = strtol(argv[2], &end, 10);
  if(errno != 0 || end == argv[2] || time < 1 || time > 30)
  {
    usage();
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x81000000);
  fifo = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x82000000);

  rst = (uint8_t *)(cfg + 0);
  cntr = (uint16_t *)(sts + 0);

  *rst &= ~1;
  *rst |= 1;

  sleep(time + 1);

  if(*cntr < time)
  {
    fprintf(stderr, "Not enough PPS pulses.\n");
    return EXIT_FAILURE;
  }

  buffer = 0;

  for(i = 0; i < time; ++i)
  {
    buffer += *fifo + 1;
  }

  corr = (freq * 1.0e6 * time / buffer - 1.0) * 1.0e6;

  if(corr < -100.0 || corr > 100.0)
  {
    fprintf(stderr, "Correction value is out of range.\n");
    return EXIT_FAILURE;
  }

  printf("%.2f\n", corr);

  return EXIT_SUCCESS;
}
