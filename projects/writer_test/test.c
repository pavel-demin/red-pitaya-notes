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
  volatile void *cfg, *sts;
  volatile uint8_t *rst;
  volatile int32_t *ram;
  volatile uint32_t *pos;
  uint32_t size;
  int32_t value;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  size = 2048*sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0)
  {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  rst = (uint8_t *)(cfg + 0);
  pos = (uint32_t *)(sts + 0);

  // set writer address
  *(uint32_t *)(cfg + 4) = size;

  printf("addr: %08x\n", size);

  for(i = 0; i < 16; ++i)
  {
    ram[i] = 0;
    ram[2097151 - 15 + i] = 0;
  }

  // reset writer
  *rst &= ~2;
  usleep(100);

  // start writer
  *rst |= 2;
  usleep(100);

  // reset counter
  *rst &= ~1;
  usleep(100);

  // set number of counts
  *(uint32_t *)(cfg + 8) = 2097151 + 64;

  // start counter
  *rst |= 1;
  usleep(100);

  printf("pos: %08x\n", *pos);

  // print counts
  for(i = 0; i < 16; ++i)
  {
    value = ram[i];
    printf("%08x\n", value);
  }

  for(i = 0; i < 16; ++i)
  {
    value = ram[2097151 - 15 + i];
    printf("%08x\n", value);
  }

  while(*pos != 2)
  {
    printf("pos: %08x\n", *pos);
    usleep(100000);
  }

  printf("pos: %08x\n", *pos);

  for(i = 0; i < 16; ++i)
  {
    value = ram[2097151 - 15 + i];
    printf("%08x\n", value);
  }

  // print counts
  for(i = 0; i < 16; ++i)
  {
    value = ram[i];
    printf("%08x\n", value);
  }

  return EXIT_SUCCESS;
}
