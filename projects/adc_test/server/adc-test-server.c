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
  volatile uint32_t *rx_addr, *rx_cntr;
  volatile uint16_t *rx_rate;
  volatile uint8_t *rx_rst;
  volatile void *cfg, *sts, *ram;
  cpu_set_t mask;
  struct sched_param param;
  struct sockaddr_in addr;
  uint32_t size;
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

  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_rate = (uint16_t *)(cfg + 2);
  rx_addr = (uint32_t *)(cfg + 4);

  rx_cntr = (uint32_t *)(sts + 0);

  *rx_addr = size;

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes));

  /* setup listening address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(TCP_PORT);

  if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(!interrupted)
  {
    /* enter reset mode */
    *rx_rst &= ~1;
    usleep(100);
    *rx_rst &= ~2;
    /* set default sample rate */
    *rx_rate = 4;

    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *rx_rst |= 2;
    usleep(100);
    *rx_rst |= 1;

    limit = 32*1024;

    while(!interrupted)
    {
      /* read ram writer position */
      position = *rx_cntr;

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
  }

  /* enter reset mode */
  *rx_rst &= ~1;
  usleep(100);
  *rx_rst &= ~2;

  close(sock_server);

  return EXIT_SUCCESS;
}
