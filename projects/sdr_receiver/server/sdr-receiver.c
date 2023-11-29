#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1001

struct control
{
  int32_t inps, rate;
  int32_t freq[8];
};

const int rates[4] = {1000, 500, 250, 250};

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int fd, i;
  int sock_server, sock_client;
  volatile void *cfg, *sts, *fifo;
  volatile uint8_t *rx_rst, *rx_sel;
  volatile uint16_t *rx_rate, *rx_cntr;
  volatile uint32_t *rx_freq;
  struct sockaddr_in addr;
  struct control ctrl;
  uint32_t size, n;
  void *buffer;
  int yes = 1;

  if((buffer = malloc(65536)) == NULL)
  {
    perror("malloc");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  fifo = mmap(NULL, 32*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_sel = (uint8_t *)(cfg + 1);
  rx_rate = (uint16_t *)(cfg + 2);
  rx_freq = (uint32_t *)(cfg + 4);

  rx_cntr = (uint16_t *)(sts + 0);

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

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
    *rx_rst &= ~1;
    *rx_sel = 0;
    *rx_rate = 1000;
    for(i = 0; i < 8; ++i)
    {
      rx_freq[i] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
    }

    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    *rx_rst |= 1;

    while(!interrupted)
    {
      if(ioctl(sock_client, FIONREAD, &size) < 0) break;

      if(size >= sizeof(struct control))
      {
        if(recv(sock_client, (char *)&ctrl, sizeof(struct control), MSG_WAITALL) < 0) break;

        /* set inputs */
        *rx_sel = ctrl.inps & 0xff;

        /* set rx sample rate */
        *rx_rate = rates[ctrl.rate & 3];

        /* set rx phase increments */
        for(i = 0; i < 8; ++i)
        {
          rx_freq[i] = (uint32_t)floor(ctrl.freq[i] / 125.0e6 * (1 << 30) + 0.5);
        }
      }

      if(*rx_cntr >= 2048)
      {
        *rx_rst &= ~1;
        *rx_rst |= 1;
      }

      if(*rx_cntr >= 1024)
      {
        memcpy(buffer, fifo, 65536);
        if(send(sock_client, buffer, 65536, MSG_NOSIGNAL) < 0) break;
      }
      else
      {
        usleep(500);
      }
    }

    signal(SIGINT, SIG_DFL);
    close(sock_client);
  }

  *rx_rst &= ~1;

  close(sock_server);

  return EXIT_SUCCESS;
}
