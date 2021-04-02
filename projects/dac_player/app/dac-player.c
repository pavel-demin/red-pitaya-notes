/*
command to compile:
gcc dac-player.c -o dac-player
*/

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  FILE *fileIn;
  int i, mmapfd;
  volatile void *cfg, *sts;
  volatile uint64_t *dac;
  char *end;
  uint64_t buffer[8192];
  long number;
  size_t size;

  if((mmapfd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40001000);
  dac = mmap(NULL, 32*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40020000);

  errno = 0;
  number = (argc == 3) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 16 || number > 16384)
  {
    fprintf(stderr, "Usage: dac-player rate file\n");
    fprintf(stderr, " rate - interpolation rate (16 - 16384),\n");
    fprintf(stderr, " file - input file.\n");
    return EXIT_FAILURE;
  }

  if((fileIn = fopen(argv[2], "rb")) < 0)
  {
    perror("fopen");
    return EXIT_FAILURE;
  }

  /* set interpolation rate */
  *((uint16_t *)(cfg + 10)) = (uint16_t)(number >> 1);

  signal(SIGINT, signal_handler);

  /* write OUT1 and OUT2 samples to FIFO */
  while(!interrupted)
  {
    if((size = fread(buffer, 1, 65536, fileIn)) <= 0) break;

    /* wait if there is not enough free space in FIFO */
    while(*((uint32_t *)(sts + 4)) > 16384)
    {
      usleep(500);
    }

    for(i = 0; i < size / 8; ++i) *dac = buffer[i];
  }

  return EXIT_SUCCESS;
}
