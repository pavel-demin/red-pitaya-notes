#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
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
  void *cfg, *sts, *ram;
  char *end, *name = "/dev/mem";
  int size = 0;
  struct sockaddr_in addr;
  uint16_t port;
  uint32_t command, freq;
  uint32_t freqMin = 50000;
  uint32_t freqMax = 60000000;
  int yes = 1;
  long number;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 0 || number > 1)
  {
    printf("Usage: sdr-receiver 0|1\n");
    return EXIT_FAILURE;
  }

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  switch(number)
  {
    case 0:
      port = 1001;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
      ram = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
      break;
    case 1:
      port = 1003;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);
      ram = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
      break;
  }

  if((sockServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sockServer, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  /* setup listening address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if(bind(sockServer, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sockServer, 1024);

  while(!interrupted)
  {
    /* set default rx phase increment */
    *((uint32_t *)(cfg + 4)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
    /* set default rx sample rate */
    *((uint32_t *)(cfg + 8)) = 625;
    /* set default tx phase increment */
    *((uint32_t *)(cfg + 12)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
    /* set default tx sample rate */
    *((uint32_t *)(cfg + 16)) = 625;

    if((sockClient = accept(sockServer, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    limit = 512;

    while(!interrupted)
    {
      if(ioctl(sockClient, FIONREAD, &size) < 0) break;

      if(size >= 4)
      {
        if(recv(sockClient, (char *)&command, 4, MSG_WAITALL) < 0) break;
        switch(command >> 28)
        {
          case 0:
            /* set rx phase increment */
            freq = command & 0xfffffff;
            if(freq < freqMin || freq > freqMax) continue;
            *((uint32_t *)(cfg + 4)) = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
            break;
          case 1:
            /* set rx sample rate */
            switch(command & 7)
            {
              case 0:
                freqMin = 10000;
                *((uint32_t *)(cfg + 8)) = 3125;
                break;
              case 1:
                freqMin = 25000;
                *((uint32_t *)(cfg + 8)) = 1250;
                break;
              case 2:
                freqMin = 50000;
                *((uint32_t *)(cfg + 8)) = 625;
                break;
              case 3:
                freqMin = 125000;
                *((uint32_t *)(cfg + 8)) = 250;
                break;
              case 4:
                freqMin = 250000;
                *((uint32_t *)(cfg + 8)) = 125;
                break;
            }
            break;
          case 2:
            /* set tx phase increment */
            freq = command & 0xfffffff;
            if(freq < freqMin || freq > freqMax) continue;
            *((uint32_t *)(cfg + 12)) = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
            break;
          case 3:
            /* set tx sample rate */
            switch(command & 7)
            {
              case 0:
                freqMin = 10000;
                *((uint32_t *)(cfg + 16)) = 3125;
                break;
              case 1:
                freqMin = 25000;
                *((uint32_t *)(cfg + 16)) = 1250;
                break;
              case 2:
                freqMin = 50000;
                *((uint32_t *)(cfg + 16)) = 625;
                break;
              case 3:
                freqMin = 125000;
                *((uint32_t *)(cfg + 16)) = 250;
                break;
              case 4:
                freqMin = 250000;
                *((uint32_t *)(cfg + 16)) = 125;
                break;
            }
            break;
        }
      }

      /* read ram writer position */
      position = *((uint16_t *)(sts + 0));

      /* send 4096 bytes if ready, otherwise sleep 0.5 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 512))
      {
        offset = limit > 0 ? 0 : 4096;
        limit = limit > 0 ? 0 : 512;
        if(send(sockClient, ram + offset, 4096, MSG_NOSIGNAL) < 0) break;
      }
      else
      {
        usleep(500);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sockClient);
  }

  close(sockServer);

  return EXIT_SUCCESS;
}
