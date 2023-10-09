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

volatile uint64_t *fifo;
volatile uint32_t *rx_freq, *tx_freq;
volatile uint16_t *rx_rate, *rx_cntr, *tx_rate, *tx_cntr;
volatile uint8_t *gpio, *rx_rst, *rx_sync, *tx_rst, *tx_sync;

int sock_thread[4] = {-1, -1, -1, -1};

void *rx_ctrl_handler(void *arg);
void *rx_data_handler(void *arg);

void *tx_ctrl_handler(void *arg);
void *tx_data_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  pthread_t thread;
  void *(*handler[4])(void *) =
  {
    rx_ctrl_handler,
    rx_data_handler,
    tx_ctrl_handler,
    tx_data_handler
  };
  volatile void *cfg, *sts;
  char *end;
  struct sockaddr_in addr;
  uint16_t port;
  uint32_t command;
  ssize_t result;
  int yes = 1;
  long number;

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 1 || number > 2)
  {
    printf("Usage: sdr-transceiver 1|2\n");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  switch(number)
  {
    case 1:
      port = 1001;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
      fifo = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
      break;
    case 2:
      port = 1002;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x80000000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x81000000);
      fifo = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x82000000);
      break;
  }

  gpio = (uint8_t *)(cfg + 2);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_freq = (uint32_t *)(cfg + 4);
  rx_sync = (uint8_t *)(cfg + 8);
  rx_rate = (uint16_t *)(cfg + 10);
  rx_cntr = (uint16_t *)(sts + 0);

  tx_rst = (uint8_t *)(cfg + 1);
  tx_freq = (uint32_t *)(cfg + 12);
  tx_sync = (uint8_t *)(cfg + 16);
  tx_rate = (uint16_t *)(cfg + 18);
  tx_cntr = (uint16_t *)(sts + 2);

  /* set PTT pin to low */
  *gpio = 0;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *rx_sync = 0;
  /* set default rx sample rate */
  *rx_rate = 640;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *tx_sync = 0;
  /* set default tx sample rate */
  *tx_rate = 640;

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
  addr.sin_port = htons(port);

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
    if(result <= 0 || command > 3 || sock_thread[command] > -1)
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
  int sock_client = sock_thread[1];
  uint8_t buffer[16384];

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

void *tx_ctrl_handler(void *arg)
{
  int sock_client = sock_thread[2];
  uint32_t command, freq;

  /* set PTT pin to low */
  *gpio = 0;
  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *tx_sync = 0;
  /* set default tx sample rate */
  *tx_rate = 640;

  while(1)
  {
    if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set tx phase increment */
        freq = command & 0xfffffff;
        *tx_freq = (uint32_t)floor(freq/122.88e6*(1<<30)+0.5);
        *tx_sync = freq > 0 ? 0 : 1;
        break;
      case 1:
        /* set tx sample rate */
        switch(command & 7)
        {
          case 0:
            *tx_rate = 2560;
            break;
          case 1:
            *tx_rate = 1280;
            break;
          case 2:
            *tx_rate = 640;
            break;
          case 3:
            *tx_rate = 320;
            break;
          case 4:
            *tx_rate = 160;
            break;
          case 5:
            *tx_rate = 80;
            break;
          case 6:
            *tx_rate = 40;
            break;
        }
        break;
      case 2:
        /* set PTT pin to high */
        *gpio = 1;
        break;
      case 3:
        /* set PTT pin to low */
        *gpio = 0;
        break;
    }
  }

  /* set PTT pin to low */
  *gpio = 0;
  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);
  *tx_sync = 0;
  /* set default tx sample rate */
  *tx_rate = 640;

  close(sock_client);
  sock_thread[2] = -1;

  return NULL;
}

void *tx_data_handler(void *arg)
{
  int i, sock_client = sock_thread[3];
  uint8_t buffer[16384];

  *tx_rst &= ~1;
  *tx_rst |= 1;

  while(1)
  {
    while(*tx_cntr > 4096) usleep(500);

    if(*tx_cntr == 0)
    {
      for(i = 0; i < 2048; ++i) *fifo = 0;
    }

    if(recv(sock_client, buffer, 16384, 0) <= 0) break;
    memcpy(fifo, buffer, 16384);
  }

  close(sock_client);
  sock_thread[3] = -1;

  return NULL;
}
