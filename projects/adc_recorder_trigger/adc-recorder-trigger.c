#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

int main()
{
  int fd, i;
  volatile uint8_t *rst;
  volatile void *cfg, *sts, *ram;
  uint32_t start, offset, size;
  int16_t value[2];

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  size = 8192*sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0)
  {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  rst = (uint8_t *)(cfg + 0);

  /* set writer address */
  *(uint32_t *)(cfg + 4) = size;

  /* reset oscilloscope and ram writer */
  *rst &= ~1;
  *rst |= 1;

  /* configure trigger edge (0 for negative, 1 for positive) */
  *(uint16_t *)(cfg + 2) = 0;

  /* set trigger mask */
  *(uint16_t *)(cfg + 8) = 1;

  /* set trigger level */
  *(uint16_t *)(cfg + 10) = 1;

  /* set number of samples before trigger */
  *(uint32_t *)(cfg + 12) = 1024 - 1;

  /* set total number of samples (up to 8 * 1024 * 1024 - 1) */
  *(uint32_t *)(cfg + 16) = 1024 * 1024 - 1;

  /* set decimation factor for CIC filter (from 5 to 3125) */
  /* combined (CIC and FIR) decimation factor is twice greater */
  *(uint16_t *)(cfg + 20) = 5;

  /* start oscilloscope */
  *rst |= 2;
  *rst &= ~2;

  /* wait when oscilloscope stops */
  while(*(uint32_t *)(sts + 0) & 1)
  {
    usleep(1000);
  }

  start = *(uint32_t *)(sts + 0) >> 1;
  start = (start - 1024) & 0x007FFFC0;

  /* print IN1 and IN2 samples */
  for(i = 0; i < 1024 * 1024; ++i)
  {
    offset = ((start + i) & 0x007FFFFF) * 4;
    value[0] = *(int16_t *)(ram + offset + 0);
    value[1] = *(int16_t *)(ram + offset + 2);
    printf("%5d %5d\n", value[0], value[1]);
  }

  return EXIT_SUCCESS;
}
