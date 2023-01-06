#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main()
{
  int fd, i;
  uint32_t value;
  volatile void *cfg, *sts;
  volatile uint8_t *rst;
  volatile uint32_t *ram, *limit;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  ram = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);

  rst = cfg + 0;
  limit = cfg + 4;

  /* stop all blocks */
  *rst &= ~3;

  /* set counter limit */
  *limit = 99;

  /* enable converter and writer */
  *rst |= 2;

  /* enable counter */
  *rst |= 1;

  usleep(1000);

  /* print values from memory */
  for(i = 0; i < 100; ++i)
  {
    value = ram[i];
    printf("%d\n", value);
  }

  return EXIT_SUCCESS;
}
