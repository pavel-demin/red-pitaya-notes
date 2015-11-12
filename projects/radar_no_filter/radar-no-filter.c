#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int main()
{
  int fd, i;
  int position, limit, offset;
  int16_t value[2];
  void *cfg, *sts, *gpio_n, *gpio_p, *ram;
  char *name = "/dev/mem";
  char buffer[32768];

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  gpio_n = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  gpio_p = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);
  ram = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);

  /* put oscilloscope and ram writer into reset mode */
  *((uint16_t *)(cfg + 0)) &= ~3;

  /* configure trigger edge (0 for negative, 1 for positive) */
  *((uint16_t *)(cfg + 2)) = 0;

  /* set trigger mask */
  *((uint16_t *)(cfg + 4)) = 1;

  /* set trigger level */
  *((uint16_t *)(cfg + 6)) = 1;

  /* set total number of samples */
  *((uint16_t *)(cfg + 8)) = 8192 - 1;

  /* set decimation factor */
  *((uint16_t *)(cfg + 10)) = 10 - 1;

  /* switch on LED 1 */
  *((uint8_t *)(cfg + 12)) |= 1;

  /* set tri-state control register */
  *((uint8_t *)(gpio_n + 4)) &= ~1;

  /* set pin DIO0_N to high */
  *((uint8_t *)(gpio_n + 0)) |= 1;

  /* set tri-state control register */
  *((uint8_t *)(gpio_p + 4)) &= ~1;

  /* set pin DIO0_P to high */
  *((uint8_t *)(gpio_p + 0)) |= 1;

  /* enter normal operating mode */
  *((uint16_t *)(cfg + 0)) |= 3;

  limit = 8192;

  while(1)
  {
    /* read ram writer position */
    position = *((uint16_t *)(sts + 2));

    /* process 8192 samples if ready, otherwise sleep */
    if((limit > 0 && position > limit) || (limit == 0 && position < 8192))
    {
      offset = limit > 0 ? 0 : 32768;
      limit = limit > 0 ? 0 : 8192;
      memcpy(buffer, ram + offset, 32768);
      /* process 8192 samples in buffer */
      for(i = 0; i < 8192; ++i)
      {
        value[0] = *((int16_t *)(buffer + 4*i + 0));
        value[1] = *((int16_t *)(buffer + 4*i + 2));
        printf("%5d %5d\n", value[0], value[1]);
      }
      return EXIT_SUCCESS;
    }
    else
    {
      usleep(100);
    }
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(gpio_n, sysconf(_SC_PAGESIZE));
  munmap(gpio_p, sysconf(_SC_PAGESIZE));
  munmap(ram, sysconf(_SC_PAGESIZE));

  return EXIT_SUCCESS;
}
