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

#define LO_MIN   100000
#define LO_MAX 50000000

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int file, sockServer, sockClient;
  int pos, limit, start;
  pid_t pid;
  void *cfg, *sts, *ram;
  char *name = "/dev/mem";
  unsigned long size = 0;
  struct sockaddr_in addrServer, addrClient;
  socklen_t lenClient;
  uint32_t command = 600000;
  int yes = 1;

  if((file = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40001000);
  ram = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40002000);

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~7;

  /* set default phase increment */
  *((uint32_t *)(cfg + 4)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);

  if((sockServer = socket(AF_INET, SOCK_STREAM, 0)) < 1)
  {
    perror("socket");
    return 1;
  }

  setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  memset(&addrServer, 0, sizeof(addrServer));
  addrServer.sin_family = AF_INET;
  addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
  addrServer.sin_port=htons(TCP_PORT);

  if(bind(sockServer, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0)
  {
    perror("bind");
    return 1;
  }

  listen(sockServer, 1024);

  lenClient = sizeof(addrClient);

  limit = 128;

  while(!interrupted)
  {
    sockClient = accept(sockServer, (struct sockaddr *)&addrClient, &lenClient);

    /* enter normal operating mode */
    *((uint32_t *)(cfg + 0)) |= 7;

    while(!interrupted)
    {
      ioctl(sockClient, FIONREAD, &size);

      if(size >= 4)
      {
        recv(sockClient, (char *)&command, 4, 0);
        if(command >= LO_MIN || command <= LO_MAX)
        {
          *((uint32_t *)(cfg + 4)) = (uint32_t)floor(command/125.0e6*(1<<30)+0.5);
        }
      }

      /* read ram writer position */
      pos = *((uint32_t *)(sts + 0));

      /* send 1024 bytes if ready, otherwise sleep 0.5 ms */
      if((limit > 0 && pos > limit) || (limit == 0 && pos < 384))
      {
        start = limit > 0 ? limit*8 - 1024 : 3072;
        if(send(sockClient, ram + start, 1024, 0) < 0) break;;
        limit += 128;
        if(limit == 512) limit = 0;
      }
      else
      {
        usleep(500);
      }
    }

    close(sockClient);

    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~7;
  }

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~7;

  return 0;
}
