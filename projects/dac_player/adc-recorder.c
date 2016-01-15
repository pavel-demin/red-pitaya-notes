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
  pid_t pid;
  FILE *fileOut;
  int pipefd[2], mmapfd;
  int position, limit, offset;
  void *cfg, *sts, *adc, *buf;
  char *end, *name = "/dev/mem";
  int buffer = 0;
  long number;

  if((mmapfd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x40001000);
  adc = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, mmapfd, 0x1E000000);
  buf = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  errno = 0;
  number = (argc == 3) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 10 || number > 65535)
  {
    printf("Usage: adc-recorder factor file\n");
    printf("factor - ADC decimation factor (10 - 65535),\n");
    printf("file - output file.\n");
    return EXIT_FAILURE;
  }

  limit = 512*1024;

  /* create a pipe */
  pipe(pipefd);

  pid = fork();
  if(pid == 0)
  {
    /* child process */

    close(pipefd[0]);

    while(1)
    {
      /* read ram writer position */
      position = *((uint32_t *)(sts + 0));

      /* write 4 MB if ready, otherwise sleep 1 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 512*1024))
      {
        offset = limit > 0 ? 0 : 4096*1024;
        limit = limit > 0 ? 0 : 512*1024;
        memcpy(buf + offset, adc + offset, 4096*1024);
        write(pipefd[1], &buffer, sizeof(buffer));
      }
      else
      {
        usleep(1000);
      }
    }
  }
  else if(pid > 0)
  {
    /* parent process */

    close(pipefd[1]);

    if((fileOut = fopen(argv[2], "wb")) < 0)
    {
      perror("fopen");
      kill(pid, SIGTERM);
      return EXIT_FAILURE;
    }

    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~3;

    /* set ADC decimation factor */
    *((uint16_t *)(cfg + 4)) = (uint16_t)number - 1;

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *((uint32_t *)(cfg + 0)) |= 3;

    while(!interrupted)
    {
      read(pipefd[0], &buffer, sizeof(buffer));
      if(fwrite(buf, 1, 4096*1024, fileOut) < 0) break;

      read(pipefd[0], &buffer, sizeof(buffer));
      if(fwrite(buf + 4096*1024, 1, 4096*1024, fileOut) < 0) break;
    }

    kill(pid, SIGTERM);

    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~3;

    fclose(fileOut);

    return EXIT_SUCCESS;
  }
}
