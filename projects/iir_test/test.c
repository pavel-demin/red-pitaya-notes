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
  uint16_t value, model, cntr, fall, rise, scale;
  int64_t y[3];
  volatile void *cfg;
  volatile uint32_t *fifo, *bram;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  bram = mmap(NULL, 16 * sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
  fifo = mmap(NULL, 16 * sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x43000000);

  *(uint8_t *)(cfg + 0) &= ~3;

  fall = (uint16_t)floor(expf(-logf(2.0) / 125.0 / 100) * 65536.0 + 0.5);
  rise = (uint16_t)floor(expf(-logf(2.0) / 125.0 / 100 * 1.0e3) * 65536.0 + 0.5);

  cntr = 0;
  y[0] = 4095 << 9;
  y[1] = 0;
  y[2] = 0;
  while(y[2] <= y[1])
  {
    ++cntr;
    y[2] = y[1];
    y[1] = y[0] + y[1] * rise / 65536;
    y[0] = y[0] * fall / 65536;
  }
  scale = (uint16_t)(4095 * 65535 / (y[2] >> 9));

  for(i = 0; i < 1024; ++i)
  {
    bram[i] = 0;
  }

  /* set pulse height */
  bram[0] = 4095;
  /* set size */
  *(uint16_t *)(cfg + 4) = 1024;
  /* set scale factor */
  *(uint16_t *)(cfg + 6) = scale;
  /* set fall time */
  *(uint16_t *)(cfg + 8) = fall;
  /* set rise time */
  *(uint16_t *)(cfg + 10) = rise;
  /* set limits */
  *(int16_t *)(cfg + 12) = -8192;
  *(int16_t *)(cfg + 14) = 8191;

  *(uint8_t *)(cfg + 0) |= 2;
  *(uint8_t *)(cfg + 0) |= 1;

  sleep(1);

  y[0] = 4095 << 9;
  y[1] = 0;
  y[2] = 0;
  y[0] *= scale;
  for(i = 0; i < cntr + 10; ++i)
  {
    value = *fifo;
    model = y[2] >> 25;
    printf("%d\t%d\t%d\n", value, model, value - model);
    if(i < 2) continue;
    y[2] = y[1];
    y[1] = y[0] + y[1] * rise / 65536;
    y[0] = y[0] * fall / 65536;
  }

  return EXIT_SUCCESS;
}
