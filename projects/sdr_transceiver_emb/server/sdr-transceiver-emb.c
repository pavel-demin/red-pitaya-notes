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

volatile uint64_t *rx_data, *tx_data;
volatile uint32_t *rx_freq, *tx_freq, *tx_mux;
volatile uint16_t *tx_level, *rx_cntr, *tx_cntr;
volatile uint8_t *rx_rst, *tx_rst, *codec_rst, *gpio;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

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
  struct sockaddr_in addr;
  uint16_t port;
  uint32_t command;
  ssize_t result;
  int yes = 1;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  port = 1001;
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);
  tx_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40005000);
  tx_mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);

  rx_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  codec_rst = ((uint8_t *)(cfg + 2));
  gpio = ((uint8_t *)(cfg + 3));

  rx_freq = ((uint32_t *)(cfg + 4));

  tx_freq = ((uint32_t *)(cfg + 24));
  tx_level = ((uint16_t *)(cfg + 30));

  rx_cntr = ((uint16_t *)(sts + 12));
  tx_cntr = ((uint16_t *)(sts + 20));

  /* set PTT pin to low */
  *gpio = 0;

  /* set default tx level */
  *tx_level = 32110;

  /* set default tx mux channel */
  *(tx_mux + 16) = 0;
  *tx_mux = 2;

  *rx_rst |= 1;
  *rx_rst &= ~1;

  *tx_rst |= 1;
  *tx_rst &= ~1;

  /* enable ALEX interface */
  *codec_rst |= 4;

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
        break;
    }
  }

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);

  close(sock_client);
  sock_thread[0] = -1;

  return NULL;
}

void *rx_data_handler(void *arg)
{
  int i, sock_client = sock_thread[1];
  uint64_t buffer[256];

  *rx_rst |= 1;
  *rx_rst &= ~1;

  while(1)
  {
    if(*rx_cntr >= 1024)
    {
      *rx_rst |= 1;
      *rx_rst &= ~1;
    }

    while(*rx_cntr < 512) usleep(1000);

    for(i = 0; i < 256; ++i) buffer[i] = *rx_data;
    if(send(sock_client, buffer, 2048, MSG_NOSIGNAL) < 0) break;
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

  while(1)
  {
    if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set tx phase increment */
        freq = command & 0xfffffff;
        if(freq < freq_min || freq > freq_max) continue;
        *tx_freq = (uint32_t)floor(freq/122.88e6*(1<<30)+0.5);
        break;
      case 1:
        /* set PTT pin */
        *gpio = command & 1;
        break;
    }
  }

  /* set PTT pin to low */
  *gpio = 0;
  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/122.88e6*(1<<30)+0.5);

  close(sock_client);
  sock_thread[2] = -1;

  return NULL;
}

void *tx_data_handler(void *arg)
{
  int i, sock_client = sock_thread[3];
  uint64_t buffer[256];

  *tx_rst |= 1;
  *tx_rst &= ~1;

  while(1)
  {
    while(*tx_cntr > 512) usleep(1000);

    if(*tx_cntr == 0)
    {
      for(i = 0; i < 256; ++i) *tx_data = 0;
    }

    if(recv(sock_client, buffer, 2048, 0) <= 0) break;
    for(i = 0; i < 256; ++i) *tx_data = buffer[i];
  }

  close(sock_client);
  sock_thread[3] = -1;

  return NULL;
}
