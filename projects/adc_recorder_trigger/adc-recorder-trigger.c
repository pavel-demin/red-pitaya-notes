#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i;
  uint32_t start, offset;
  int16_t value[2];
  volatile uint32_t *slcr, *axi_hp0;
  volatile void *cfg, *sts, *ram;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  axi_hp0 = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8008000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);

  /* set HP0 bus width to 64 bits */
  slcr[2] = 0xDF0D;
  slcr[144] = 0;
  axi_hp0[0] &= ~1;
  axi_hp0[5] &= ~1;

  /* reset oscilloscope and ram writer */
  *((uint16_t *)(cfg + 0)) &= ~1;
  *((uint16_t *)(cfg + 0)) |= 1;

  /* configure trigger edge (0 for negative, 1 for positive) */
  *((uint16_t *)(cfg + 2)) = 0;

  /* set trigger mask */
  *((uint16_t *)(cfg + 4)) = 1;

  /* set trigger level */
  *((uint16_t *)(cfg + 6)) = 1;

  /* set number of samples before trigger */
  *((uint32_t *)(cfg + 8)) = 1024 - 1;

  /* set total number of samples (up to 8 * 1024 * 1024 - 1) */
  *((uint32_t *)(cfg + 12)) = 1024 * 1024 - 1;

  /* set decimation factor for CIC filter (from 5 to 3125) */
  /* combined (CIC and FIR) decimation factor is twice greater */
  *((uint16_t *)(cfg + 16)) = 5;

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

  /* print IN1 and IN2 samples */
  for(i = 0; i < 1024 * 1024; ++i)
  {
    offset = ((start + i) & 0x007FFFFF) * 4;
    value[0] = *((int16_t *)(ram + offset + 0));
    value[1] = *((int16_t *)(ram + offset + 2));
    printf("%5d %5d\n", value[0], value[1]);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(ram, sysconf(_SC_PAGESIZE));

  return 0;
}
