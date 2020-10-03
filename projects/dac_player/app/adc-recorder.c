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
#include <sys/mman.h>
#include <fcntl.h>

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

void neoncopy(void *dst, volatile void *src, int cnt)
{
  asm volatile
  (
    "loop_%=:\n"
    "vldm %[src]!, {q0, q1, q2, q3}\n"
    "vstm %[dst]!, {q0, q1, q2, q3}\n"
    "subs %[cnt], %[cnt], #64\n"
    "bgt loop_%="
    : [dst] "+r" (dst), [src] "+r" (src), [cnt] "+r" (cnt)
    :
    : "q0", "q1", "q2", "q3", "cc", "memory"
  );
}

int main(int argc, char *argv[])
{
  FILE *fileOut;
  int mmapfd;
  int position, limit, offset;
  volatile uint32_t *slcr, *axi_hp0;
  volatile void *cfg, *sts, *adc;
  void *buf;
  cpu_set_t mask;
  struct sched_param param;
  char *end;
  int buffer = 0;
  long number;

  errno = 0;
  number = (argc == 3) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 10 || number > 65535)
  {
    printf("Usage: adc-recorder factor file\n");
    printf("factor - ADC decimation factor (10 - 65535),\n");
    printf("file - output file.\n");
    return EXIT_FAILURE;
  }

  if((fileOut = fopen(argv[2], "wb")) < 0)
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

  if((mmapfd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0xF8000000);
  axi_hp0 = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0xF8008000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40001000);
  adc = mmap(NULL, 128*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x1E000000);
  buf = mmap(NULL, 64*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  /* set HP0 bus width to 64 bits */
  slcr[2] = 0xDF0D;
  slcr[144] = 0;
  axi_hp0[0] &= ~1;
  axi_hp0[5] &= ~1;

  /* enter reset mode */
  *(uint8_t *)(cfg + 0) &= ~1;
  usleep(100);
  *(uint8_t *)(cfg + 0) &= ~2;

  /* set ADC decimation factor */
  *(uint16_t *)(cfg + 2) = (uint16_t)number - 1;

  signal(SIGINT, signal_handler);

  /* enter normal operating mode */
  *(uint8_t *)(cfg + 0) |= 3;

  limit = 32*1024;

  while(!interrupted)
  {
    /* read ADC writer position */
    position = *(uint32_t *)(sts + 0);

    /* send 256 kB if ready, otherwise sleep 0.1 ms */
    if((limit > 0 && position > limit) || (limit == 0 && position < 32*1024))
    {
      offset = limit > 0 ? 0 : 256*1024;
      limit = limit > 0 ? 0 : 32*1024;
      neoncopy(buf, adc + offset, 256*1024);
      if(fwrite(buf, 1, 256*1024, fileOut) < 0) break;
    }
    else
    {
      usleep(100);
    }
  }

  /* enter reset mode */
  *(uint8_t *)(cfg + 0) &= ~1;
  usleep(100);
  *(uint8_t *)(cfg + 0) &= ~2;

  fclose(fileOut);

  return EXIT_SUCCESS;
}
