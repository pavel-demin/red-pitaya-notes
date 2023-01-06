#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(int argc, char *argv[])
{
  int fd;
  volatile void *cfg, *sts;
  volatile uint8_t *c8, *s8;
  volatile uint16_t *c16, *s16;
  volatile uint32_t *c32, *s32;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    fprintf(stderr, "Cannot open /dev/mem.\n");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);

  c8 = cfg;
  c16 = cfg;
  c32 = cfg;

  s8 = sts;
  s16 = sts;
  s32 = sts;

  c8[0] = 0x01;
  c8[1] = 0x02;
  c8[2] = 0x04;
  c8[3] = 0x08;

  printf("0x%08x\n", s16[0]);
  printf("0x%08x\n", s16[1]);
  printf("0x%08x\n", s32[0]);

  c16[2] = 0x2010;
  c16[3] = 0x8040;

  printf("0x%08x\n", s32[1]);

  c32[2] = 0x88442211;

  printf("0x%08x\n", s32[2]);

  return EXIT_SUCCESS;
}
