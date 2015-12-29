#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i, j, size, counter;
  int16_t value[2];
  void *cfg, *sts, *dac, *adc;
  char *name = "/dev/mem";

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  adc = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  dac = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  // configure trigger edge (0 for negative, 1 for positive)
  *(uint8_t *)(cfg + 1) = 0;

  // set ADC decimation factor
  *(uint16_t *)(cfg + 2) = 4096;

  // set number of ADC samples per pixel (4 samples after decimation)
  *(uint32_t *)(cfg + 4) = 4 * 4096 - 1;

  // stop trigger, oscilloscope and CIC filter
  *(uint8_t *)(cfg + 0) |= 4;

  // reset ADC and DAC FIFO
  *(uint8_t *)(cfg + 0) |= 3;
  *(uint8_t *)(cfg + 0) &= ~3;

  counter = 0;

  // read IN1 and IN2 samples from ADC FIFO
  // write OUT1 and OUT2 samples to DAC FIFO
  for(i = 0; i <= 8176; i += 16)
  {
    size = *(uint16_t *)(sts + 0);
    for(i = 0; i < size; ++i)
    {
      ++counter;
      memcpy(value, adc, 4);
      printf("%8d %8d\n", value[0], value[1]);
    }

    // wait if there is not enough free space in DAC FIFO
    if(*(uint16_t *)(sts + 2) > 15360)
    {
      usleep(1000);
      continue;
    }

    for(j = 0; j <= 8176; j += 16)
    {
      value[0] = i;
      value[1] = j;
      memcpy(dac, value, 4);
    }

    i += 16;

    for(j = 8176; j >= 0; j -= 16)
    {
      value[0] = i;
      value[1] = j;
      memcpy(dac, value, 4);
    }

    // start trigger, oscilloscope and CIC filter
    if(i == 0) *(uint8_t *)(cfg + 0) &= ~4;
  }

  // read remaining IN1 and IN2 samples from ADC FIFO
  while(counter < 1024 * 1024 - 1)
  {
    size = *(uint16_t *)(sts + 0);
    for(i = 0; i < size; ++i)
    {
      ++counter;
      memcpy(value, adc, 4);
      printf("%8d %8d\n", value[0], value[1]);
    }
    usleep(1000);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(adc, sysconf(_SC_PAGESIZE));
  munmap(dac, sysconf(_SC_PAGESIZE));

  return 0;
}
