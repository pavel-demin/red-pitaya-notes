#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1001

#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main ()
{
  int fd, sock_server, sock_client;
  int position, limit, offset;
  volatile uint32_t *addr, *freq, *cntr;
  volatile uint16_t *rate, *level;
  volatile uint8_t *rst;
  volatile void *cfg, *sts, *ram;
  cpu_set_t mask;
  struct sched_param param;
  struct sockaddr_in addr_server;
  uint32_t command, size, addr_dma;
  int32_t value;
  int yes = 1;

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
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  size = 2048*sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0)
  {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  addr_dma = size;

  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  rst = (uint8_t *)(cfg + 0);
  rate = (uint16_t *)(cfg + 2);
  addr = (uint32_t *)(cfg + 4);
  freq = (uint32_t *)(cfg + 8);
  level = (uint16_t *)(cfg + 40);

  cntr = (uint32_t *)(sts + 0);

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

  /* setup listening address */
  memset(&addr_server, 0, sizeof(addr_server));
  addr_server.sin_family = AF_INET;
  addr_server.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_server.sin_port = htons(TCP_PORT);

  if(bind(sock_server, (struct sockaddr *)&addr_server, sizeof(addr_server)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(!interrupted)
  {
    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    /* set default sample rate */
    *rate = 20;

    *addr = addr_dma;

    /* set default phase increments */
    freq[0] = (uint32_t)floor(10000000 / 125.0e6 * (1<<30) + 0.5);
    freq[1] = 0;
    freq[2] = (uint32_t)floor(10000000 / 125.0e6 * (1<<30) + 0.5);
    freq[3] = 0;
    freq[4] = (uint32_t)floor(10000000 / 125.0e6 * (1<<30) + 0.5);
    freq[5] = 0;
    freq[6] = (uint32_t)floor(10000000 / 125.0e6 * (1<<30) + 0.5);
    freq[7] = 0;

    /* enter normal operating mode */
    *rst |= 2;

    limit = 32*1024;

    while(!interrupted)
    {
      if(ioctl(sock_client, FIONREAD, &size) < 0) break;

      if(size >= 4)
      {
        if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) < 0) break;
        value = command & 0xfffffff;
        switch(command >> 28)
        {
          case 0:
            /* set sample rate */
            if(value < 16 || value > 8192) continue;
            *rate = value;
            break;
          case 1:
            /* set rx1 phase increment */
            if(value < 0 || value > 62500000) continue;
            freq[0] = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
            freq[1] = value > 0 ? 0 : 1;
            break;
          case 2:
            /* set rx2 phase increment */
            if(value < 0 || value > 62500000) continue;
            freq[2] = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
            freq[3] = value > 0 ? 0 : 1;
            break;
          case 3:
            /* set tx1 phase increment */
            if(value < 0 || value > 62500000) continue;
            freq[4] = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
            freq[5] = value > 0 ? 0 : 1;
            break;
          case 4:
            /* set tx2 phase increment */
            if(value < 0 || value > 62500000) continue;
            freq[6] = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
            freq[7] = value > 0 ? 0 : 1;
            break;
          case 5:
            /* set tx1 level */
            if(value < 0 || value > 32766) continue;
            level[0] = value;
            break;
          case 6:
            /* set tx2 level */
            if(value < 0 || value > 32766) continue;
            level[1] = value;
            break;
        }
      }

      /* read ram writer position */
      position = *cntr;

      /* send 4 MB if ready, otherwise sleep 1 ms */
      if((limit > 0 && position > limit) || (limit == 0 && position < 32*1024))
      {
        offset = limit > 0 ? 0 : 4096*1024;
        limit = limit > 0 ? 0 : 32*1024;
        if(send(sock_client, ram + offset, 4096*1024, MSG_NOSIGNAL) < 0) break;
      }
      else
      {
        usleep(1000);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sock_client);

    /* enter reset mode */
    usleep(100);
    *rst &= ~2;
  }

  /* enter reset mode */
  usleep(100);
  *rst &= ~2;

  close(sock_server);

  return EXIT_SUCCESS;
}
