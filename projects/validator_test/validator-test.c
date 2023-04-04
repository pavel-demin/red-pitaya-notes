#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main()
{
  int fd, i;
  int position, limit, offset;
  int16_t value[2];
  volatile void *cfg, *sts, *ram;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  ram = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  signal(SIGINT, signal_handler);

  limit = 512;

  /* reset fifo and writer */
  *(uint32_t *)(cfg + 0) &= ~1;
  *(uint32_t *)(cfg + 0) |= 1;

  while(!interrupted)
  {
    /* read writer position */
    position = *(uint32_t *)(sts + 0);

    /* print 512 IN1 and IN2 samples if ready, otherwise sleep 1 ms */
    if((limit > 0 && position > limit) || (limit == 0 && position < 512))
    {
      offset = limit > 0 ? 0 : 2048;
      limit = limit > 0 ? 0 : 512;

      for(i = 0; i < 512; ++i)
      {
        value[0] = *(int16_t *)(ram + offset + 4*i + 0);
        value[1] = *(int16_t *)(ram + offset + 4*i + 2);
        printf("%5d %5d\n", value[0], value[1]);
      }
    }
    else
    {
      usleep(1000);
    }
  }

  offset = limit > 0 ? 0 : 2048;
  limit = limit > 0 ? position : position - 512;

  /* print last IN1 and IN2 samples */
  for(i = 0; i < limit; ++i)
  {
    value[0] = *(int16_t *)(ram + 4*i + 0);
    value[1] = *(int16_t *)(ram + 4*i + 2);
    printf("%5d %5d\n", value[0], value[1]);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(ram, sysconf(_SC_PAGESIZE));

  return 0;
}

