#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

volatile uint8_t *rx_rst, *rx_sel, *rx_sync;
volatile uint16_t *rx_rate, *rx_cntr;
volatile uint32_t *rx_freq, *fifo;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int sock_thread[2] = {-1, -1};

void *rx_ctrl_handler(void *arg);
void *rx_data_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  pthread_t thread;
  void *(*handler[2])(void *) =
  {
    rx_ctrl_handler,
    rx_data_handler
  };
  volatile void *cfg, *sts;
  char *end;
  struct sockaddr_in addr;
  uint32_t command;
  ssize_t result;
  int yes = 1;
  uint8_t chan = 0;
  long number;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 1 || number > 2)
  {
    fprintf(stderr, "Usage: sdr-receiver 1|2\n");
    return EXIT_FAILURE;
  }

  chan = number - 1;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x81000000);
  fifo = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x82000000);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_sel = (uint8_t *)(cfg + 1);
  rx_rate = (uint16_t *)(cfg + 2);
  rx_freq = (uint32_t *)(cfg + 4);
  rx_sync = (uint8_t *)(cfg + 8);
  rx_cntr = (uint16_t *)(sts + 0);

  *rx_sel = chan;

  /* set default rx sample rate */
  *rx_rate = 640;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *rx_sync = 0;

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
  addr.sin_port = htons(1001);

  if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(1)
  {
    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    result = recv(sock_client, (char *)&command, 4, MSG_WAITALL);
    if(result <= 0 || command > 1 || sock_thread[command] > -1)
    {
      close(sock_client);
      continue;
    }

    sock_thread[command] = sock_client;

    if(pthread_create(&thread, NULL, handler[command], NULL) < 0)
    {
      perror("pthread_create");
      return EXIT_FAILURE;
    }
    pthread_detach(thread);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}

void *rx_ctrl_handler(void *arg)
{
  int sock_client = sock_thread[0];
  uint32_t command, freq;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *rx_sync = 0;
  /* set default rx sample rate */
  *rx_rate = 640;

  while(1)
  {
    if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set rx phase increment */
        freq = command & 0xfffffff;
        if(freq < freq_min || freq > freq_max) continue;
        *rx_freq = (uint32_t)floor(freq/122.88e6*(1<<30)+0.5);
        *rx_sync = freq > 0 ? 0 : 1;
        break;
      case 1:
        /* set rx sample rate */
        switch(command & 7)
        {
          case 0:
            *rx_rate = 2560;
            break;
          case 1:
            *rx_rate = 1280;
            break;
          case 2:
            *rx_rate = 640;
            break;
          case 3:
            *rx_rate = 320;
            break;
          case 4:
            *rx_rate = 160;
            break;
          case 5:
            *rx_rate = 80;
            break;
          case 6:
            *rx_rate = 40;
            break;
        }
        break;
    }
  }

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *rx_sync = 0;
  /* set default rx sample rate */
  *rx_rate = 640;

  close(sock_client);
  sock_thread[0] = -1;

  return NULL;
}

void *rx_data_handler(void *arg)
{
  int i, sock_client = sock_thread[1];
  uint64_t buffer[2048];

  *rx_rst &= ~1;
  *rx_rst |= 1;

  while(1)
  {
    if(*rx_cntr >= 8192)
    {
      *rx_rst &= ~1;
      *rx_rst |= 1;
    }

    while(*rx_cntr < 4096) usleep(500);

    memcpy(buffer, fifo, 16384);
    if(send(sock_client, buffer, 16384, MSG_NOSIGNAL) < 0) break;
  }

  close(sock_client);
  sock_thread[1] = -1;

  return NULL;
}
