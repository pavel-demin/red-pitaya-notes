#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i, j;
  int16_t value[2];
  volatile uint32_t *slcr, *axi_hp0;
  volatile void *cfg, *sts, *dac, *adc;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  axi_hp0 = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8008000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  dac = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  adc = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);

  // set HP0 bus width to 64 bits
  slcr[2] = 0xDF0D;
  slcr[144] = 0;
  axi_hp0[0] &= ~1;
  axi_hp0[5] &= ~1;

  // set ADC decimation factor (125e6/5/1024/8)
  *((uint16_t *)(cfg + 4)) = 3052;

  // set DAC interpolation factor (125e6/5/1024)
  *((uint16_t *)(cfg + 6)) = 24416 - 1;

  // reset RAM writer
  *((uint16_t *)(cfg + 0)) &= ~2;
  *((uint16_t *)(cfg + 0)) |= 2;

  // enter reset mode for packetizer
  *((uint16_t *)(cfg + 0)) &= ~1;

  // set number of ADC samples
  *((uint32_t *)(cfg + 8)) = 4098 * 512 - 1;

  // enter normal mode (start recording ADC samples)
  *((uint16_t *)(cfg + 0)) |= 1;

  // wait for RAM writer
  while(*((uint32_t *)(sts + 0)) < 32)
  {
    usleep(500);
  }

  // write OUT1 and OUT2 samples to DAC FIFO
  for(i = 0; i <= 8176; i += 16)
  {
    // wait if there is not enough free space in FIFO
    while(*((uint32_t *)(sts + 4)) > 15000)
    {
      usleep(10000);
    }

    for(j = 0; j <= 8176; j += 16)
    {
      value[0] = j;
      value[1] = i;
      memcpy(dac, value, 4);
    }

    i += 16;

    for(j = 8176; j >= 0; j -= 16)
    {
      value[0] = j;
      value[1] = i;
      memcpy(dac, value, 4);
    }
  }

  // hold last position slightly longer
  for(i = 0; i < 3; ++i)
  {
    value[0] = 0;
    value[1] = 8176;
    memcpy(dac, value, 4);
  }

  // wait for RAM writer
  while(*((uint32_t *)(sts + 0)) < 2049 * 512)
  {
    usleep(10000);
  }

  // print IN1 and IN2 samples
  for(i = 0; i < 4 * 4098 * 512; i += 4)
  {
    memcpy(value, adc + i, 4);
    printf("%8d %8d\n", value[0], value[1]);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(dac, sysconf(_SC_PAGESIZE));
  munmap(adc, sysconf(_SC_PAGESIZE));

  return 0;
}
