#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i;
  uint32_t start, offset;
  int32_t value;
  void *cfg, *sts, *ram;
  char *name = "/dev/mem";

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);

  /* reset oscilloscope and ram writer */
  *((uint16_t *)(cfg + 0)) &= ~1;
  *((uint16_t *)(cfg + 0)) |= 1;

  /* set default phase increment */
  *((uint32_t *)(cfg + 4)) = (uint32_t)floor(13560000/125.0e6*(1<<30)+0.5);

  /* set default sample rate */
  *((uint32_t *)(cfg + 8)) = 10;

  /* configure trigger edge (0 for negative, 1 for positive) */
  *((uint16_t *)(cfg + 2)) = 1;

  /* set trigger level */
  *((int32_t *)(cfg + 12)) = 10000;

  /* set number of samples before trigger */
  *((uint32_t *)(cfg + 16)) = 1024 - 1;

  /* set total number of samples */
  *((uint32_t *)(cfg + 20)) = 8 * 1024 * 1024 - 1;

  /* start oscilloscope */
  *((uint16_t *)(cfg + 0)) |= 2;
  *((uint16_t *)(cfg + 0)) &= ~2;

  /* wait when oscilloscope stops */
  while(*((uint32_t *)(sts + 0)) & 1)
  {
    usleep(1000);
  }

  start = *((uint32_t *)(sts + 0)) >> 1;
  start = (start - 1024) & 0x007FFFC0;

  /* print (I^2 + Q^2) values */
  for(i = 0; i < 8 * 1024 * 1024; ++i)
  {
    offset = ((start + i) & 0x007FFFFF) * 4;
    value = *((int32_t *)(ram + offset));
    printf("%10d\n", value);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(ram, sysconf(_SC_PAGESIZE));

  return 0;
}
