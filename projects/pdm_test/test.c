#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <sys/mman.h>

int main ()
{
  int fd, i;
  volatile void *cfg;
  volatile uint32_t *fifo;
  volatile uint16_t *sts, *level;
  volatile uint8_t *rst;
  int32_t value;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  fifo = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  rst = (uint8_t *)(cfg + 0);
  level = (uint16_t *)(cfg + 2);

  *level = 21990;

  *rst &= ~3;

  *rst |= 2;

  for(i = 0; i < 10; ++i) *fifo = 0;
  usleep(100);
  printf("%d %d\n", sts[0], sts[1]);

  *rst |= 1;

  while(1)
  {
    while(sts[0] > 544) usleep(100);

    for(i = 0; i < 480; ++i)
    {
      value = (int32_t)floor(sinf(2.0 * M_PI * i / 48) * 32000 + 0.5);
      *fifo = value;
    }

    printf("%d %d\n", sts[0], sts[1]);
  }

  return EXIT_SUCCESS;
}
