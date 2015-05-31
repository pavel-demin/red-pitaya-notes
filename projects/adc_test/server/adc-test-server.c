/*
command to compile:
gcc -O3 adc-test-server.c -o adc-test-server
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1001

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main ()
{
  int pipes[2];
  pid_t pid;
  int files[2], sockServer, sockClient;
  int position, limit, offset;
  void *cfg, *sts, *ram, *buf;
  char *name;
  unsigned long size = 0;
  struct sockaddr_in addr;
  int yes = 1;

  name = "/dev/mem";
  if((files[0] = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, files[0], 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, files[0], 0x40001000);
  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, files[0], 0x1E000000);

  name = "/dev/zero";
  if((files[1] = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  buf = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, files[1], 0);
  if(buf == MAP_FAILED)
  {
    perror("mmap");
    return 1;
  }

  limit = 512*1024;

  /* create a pipe */
  pipe(pipes);

  pid = fork();
  if(pid == 0)
  {
    /* child process */
    FILE *stream;
    char buffer[256];

    close(pipes[1]);
    stream = fdopen(pipes[0], "r");

    if((sockServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror("socket");
      return 1;
    }

    setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

    /* setup listening address */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(TCP_PORT);

    if(bind(sockServer, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
      perror("bind");
      return 1;
    }

    listen(sockServer, 1024);

    while(1)
    {
      /* enter reset mode */
      *((uint32_t *)(cfg + 0)) &= ~15;
      /* set default sample rate */
      *((uint32_t *)(cfg + 4)) = 4;

      if((sockClient = accept(sockServer, NULL, NULL)) < 0)
      {
        perror("accept");
        return 1;
      }

      printf("new connection\n");

      /* enter normal operating mode */
      *((uint32_t *)(cfg + 0)) |= 15;

      while(1)
      {
        fgets(buffer, sizeof(buffer), stream);
        if(send(sockClient, buf, 4096*1024, 0) < 0) return;

        fgets(buffer, sizeof(buffer), stream);
        if(send(sockClient, buf + 4096*1024, 4096*1024, 0) < 0) return;
      }
    }
  }
  else if(pid > 0)
  {
    /* parent process */
    FILE *stream;

    close(pipes[0]);
    stream = fdopen(pipes[1], "w");

    while(1)
    {
      /* read ram writer position */
      position = *((uint32_t *)(sts + 0));

      /* send 4 MB bytes if ready, otherwise sleep 1 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 512*1024))
      {
        offset = limit > 0 ? 0 : 4096*1024;
        limit = limit > 0 ? 0 : 512*1024;
        memcpy(buf + offset, ram + offset, 4096*1024);
        fprintf(stream, "\n");
        fflush(stream);
      }
      else
      {
        usleep(1000);
      }
    }
  }
}
