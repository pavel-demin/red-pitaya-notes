/*
command to compile:
gcc adc-recorder.c -o adc-recorder
*/

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

void usage()
{
  fprintf(stderr, "Usage: adc-recorder rate time file\n");
  fprintf(stderr, " rate - decimation rate (16 - 16384),\n");
  fprintf(stderr, " time - duration of the acquisition in seconds (from 1 to 99999999),\n");
  fprintf(stderr, " file - output file.\n");
}

int main(int argc, char *argv[])
{
  FILE *fileOut;
  int fd;
  uint32_t position, limit, offset, size;
  volatile void *cfg, *sts, *adc;
  cpu_set_t mask;
  struct sched_param param;
  char *end;
  long value;
  int32_t rate;
  time_t start, stop;

  if(argc != 4)
  {
    usage();
    return EXIT_FAILURE;
  }

  errno = 0;
  value = strtol(argv[1], &end, 10);
  if(errno != 0 || end == argv[1] || value < 16 || value > 16384)
  {
    usage();
    return EXIT_FAILURE;
  }
  rate = value;

  errno = 0;
  value = strtol(argv[2], &end, 10);
  if(errno != 0 || end == argv[2] || value < 1 || value > 99999999)
  {
    usage();
    return EXIT_FAILURE;
  }
  stop = value;

  if((fileOut = fopen(argv[3], "wb")) < 0)
  {
    perror("fopen");
    return EXIT_FAILURE;
  }

  memset(&param, 0, sizeof(param));
  param.sched_priority = sched_get_priority_max(SCHED_FIFO);
  sched_setscheduler(0, SCHED_FIFO, &param);

  CPU_ZERO(&mask);
  CPU_SET(1, &mask);
  sched_setaffinity(0, sizeof(cpu_set_t), &mask);

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  size = 128*sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0)
  {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  adc = mmap(NULL, 128*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  /* set writer address */
  *(uint32_t *)(cfg + 4) = size;

  /* enter reset mode */
  *(uint8_t *)(cfg + 0) &= ~1;
  usleep(100);
  *(uint8_t *)(cfg + 0) &= ~2;

  /* set decimation rate */
  *(uint16_t *)(cfg + 8) = (uint16_t)(rate >> 1);

  start = time(NULL) + 1;

  signal(SIGINT, signal_handler);

  /* enter normal operating mode */
  *(uint8_t *)(cfg + 0) |= 3;

  limit = 32*1024;

  while(!interrupted && time(NULL) - start < stop)
  {
    /* read writer position */
    position = *(uint32_t *)(sts + 0);

    /* write 256 kB if ready, otherwise sleep 0.1 ms */
    if((limit > 0 && position > limit) || (limit == 0 && position < 32*1024))
    {
      offset = limit > 0 ? 0 : 256*1024;
      limit = limit > 0 ? 0 : 32*1024;
      if(fwrite(adc + offset, 1, 256*1024, fileOut) < 0) break;
    }
    else
    {
      usleep(100);
    }
  }

  /* stop acquisition */
  *(uint8_t *)(cfg + 0) &= ~1;
  usleep(100);

  /* read writer position */
  position = *(uint32_t *)(sts + 0);

  /* write remaining data */
  if((limit > 0 && position < limit) || (limit == 0 && position > 32*1024))
  {
    offset = limit > 0 ? 0 : 256*1024;
    fwrite(adc + offset, 1, position * 8 - offset, fileOut);
  }

  *(uint8_t *)(cfg + 0) &= ~2;

  fclose(fileOut);

  return EXIT_SUCCESS;
}
