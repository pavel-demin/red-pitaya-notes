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
  volatile void *cfg, *sts;
  volatile uint32_t *wr, *rd;
  volatile uint8_t *rst;
  volatile uint32_t *freq;
  uint32_t buffer;
  int16_t value;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  wr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rd = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);

  rst = cfg + 0;
  freq = cfg + 4;

  *rst &= ~6;

  *freq = (uint32_t)floor(25.0 / 125.0 * (1<<30) + 0.5);

  *rst |= 1;
  *rst &= ~1;

  *wr = 32766;
  *wr = (uint32_t)floor(0.25 * (1<<30) + 0.5);
  *wr = 16-2;
  *wr = 5;

  *wr = 0;
  *wr = 0;
  *wr = 12-2;
  *wr = 1;

  *wr = 32766;
  *wr = (uint32_t)floor(0.25 * (1<<30) + 0.5);
  *wr = 16-2;
  *wr = 5;

  *rst |= 4;
  *rst |= 2;

  sleep(1);

  for(i = 0; i < 100; ++i)
  {
    buffer = *rd;
    value = buffer & 0xffff;
    printf("%6d %d %d\n", value, (buffer >> 16) & 1, (buffer >> 24) & 1);
  }

  return EXIT_SUCCESS;
}
