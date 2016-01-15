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

int main(int argc, char *argv[])
{
  int fd, sockServer, sockClient;
  int position, limit, offset;
  pid_t pid;
  void *cfg, *sts, *ram, *buf;
  char *name = "/dev/mem";
  unsigned long size = 0;
  struct sockaddr_in addr;
  uint32_t command = 13560000;
  uint32_t freqMin = 3125000;
  uint32_t freqMax = 50000000;
  int yes = 1;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  ram = mmap(NULL, 64*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);
  buf = mmap(NULL, 64*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

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

  limit = 16*1024;

  while(!interrupted)
  {
    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~7;
    /* set default phase increment */
    *((uint32_t *)(cfg + 4)) = (uint32_t)floor(13560000/125.0e6*(1<<30)+0.5);
    /* set default sample rate */
    *((uint32_t *)(cfg + 8)) = 10;

    if((sockClient = accept(sockServer, NULL, NULL)) < 0)
    {
      perror("accept");
      return 1;
    }

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *((uint32_t *)(cfg + 0)) |= 7;

    while(!interrupted)
    {
      if(ioctl(sockClient, FIONREAD, &size) < 0) break;

      if(size >= 4)
      {
        if(recv(sockClient, (char *)&command, 4, MSG_WAITALL) < 0) break;
        switch(command >> 31)
        {
          case 0:
            /* set phase increment */
            if(command < freqMin || command > freqMax) continue;
            *((uint32_t *)(cfg + 4)) = (uint32_t)floor(command/125.0e6*(1<<30)+0.5);
            break;
          case 1:
            /* set sample rate */
            switch(command & 3)
            {
              case 0:
                freqMin = 1250000;
                *((uint32_t *)(cfg + 0)) &= ~4;
                *((uint32_t *)(cfg + 8)) = 25;
                *((uint32_t *)(cfg + 0)) |= 4;
                break;
              case 1:
                freqMin = 3125000;
                *((uint32_t *)(cfg + 0)) &= ~4;
                *((uint32_t *)(cfg + 8)) = 10;
                *((uint32_t *)(cfg + 0)) |= 4;
                break;
            }
            break;
        }
      }

      /* read ram writer position */
      position = *((uint32_t *)(sts + 0));

      /* send 128 kB if ready, otherwise sleep 1 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 16*1024))
      {
        offset = limit > 0 ? 0 : 128*1024;
        limit = limit > 0 ? 0 : 16*1024;
        /* copy data to cached buffer */
        memcpy(buf + offset, ram + offset, 128*1024);
        if(send(sockClient, buf + offset, 128*1024, MSG_NOSIGNAL) < 0) break;
      }
      else
      {
        usleep(1000);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sockClient);
  }

  close(sockServer);

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~7;

  return 0;
}
