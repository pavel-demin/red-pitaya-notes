#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

uint32_t *rx_freq, *rx_rate, *tx_freq, *tx_rate;
uint16_t *gpio, *rx_cntr, *tx_cntr;
void *rx_data, *tx_data;

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
  void *cfg, *sts;
  char *end, *name = "/dev/mem";
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

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  switch(number)
  {
    case 1:
      port = 1001;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
      rx_data = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
      tx_data = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
      break;
    case 2:
      port = 1002;
      cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
      sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);
      rx_data = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
      tx_data = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000A000);
      break;
  }

  gpio = ((uint16_t *)(cfg + 0));

  rx_freq = ((uint32_t *)(cfg + 4));
  rx_rate = ((uint32_t *)(cfg + 8));
  rx_cntr = ((uint16_t *)(sts + 0));

  tx_freq = ((uint32_t *)(cfg + 12));
  tx_rate = ((uint32_t *)(cfg + 16));
  tx_cntr = ((uint16_t *)(sts + 2));

  /* set PTT pin to low */
  *gpio = 0;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate = 625;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *tx_rate = 625;

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
  uint32_t freq_min = 50000;
  uint32_t freq_max = 60000000;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate = 625;

  while(1)
  {
    if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set rx phase increment */
        freq = command & 0xfffffff;
        if(freq < freq_min || freq > freq_max) continue;
        *rx_freq = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
        break;
      case 1:
        /* set rx sample rate */
        switch(command & 7)
        {
          case 0:
            freq_min = 10000;
            *rx_rate = 3125;
            break;
          case 1:
            freq_min = 25000;
            *rx_rate = 1250;
            break;
          case 2:
            freq_min = 50000;
            *rx_rate = 625;
            break;
          case 3:
            freq_min = 125000;
            *rx_rate = 250;
            break;
          case 4:
            freq_min = 250000;
            *rx_rate = 125;
            break;
        }
        break;
    }
  }

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate = 625;

  close(sock_client);
  sock_thread[0] = -1;

  return NULL;
}

void *rx_data_handler(void *arg)
{
  int sock_client = sock_thread[1];
  int position, limit, offset;
  char buffer[4096];

  limit = 512;

  while(1)
  {
    /* read ram writer position */
    position = *rx_cntr;

    /* send 4096 bytes if ready, otherwise sleep */
    if((limit > 0 && position > limit) || (limit == 0 && position < 512))
    {
      offset = limit > 0 ? 0 : 4096;
      limit = limit > 0 ? 0 : 512;
      memcpy(buffer, rx_data + offset, 4096);
      if(send(sock_client, buffer, 4096, MSG_NOSIGNAL) < 0) break;
    }
    else
    {
      usleep(*rx_rate * 2);
    }
  }

  close(sock_client);
  sock_thread[1] = -1;

  return NULL;
}

void *tx_ctrl_handler(void *arg)
{
  int sock_client = sock_thread[2];
  uint32_t command, freq;
  uint32_t freq_min = 50000;
  uint32_t freq_max = 60000000;

  /* set PTT pin to low */
  *gpio = 0;
  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *tx_rate = 625;

  while(1)
  {
    if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
    switch(command >> 28)
    {
      case 0:
        /* set tx phase increment */
        freq = command & 0xfffffff;
        if(freq < freq_min || freq > freq_max) continue;
        *tx_freq = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
        break;
      case 1:
        /* set tx sample rate */
        switch(command & 7)
        {
          case 0:
            freq_min = 10000;
            *tx_rate = 3125;
            break;
          case 1:
            freq_min = 25000;
            *tx_rate = 1250;
            break;
          case 2:
            freq_min = 50000;
            *tx_rate = 625;
            break;
          case 3:
            freq_min = 125000;
            *tx_rate = 250;
            break;
          case 4:
            freq_min = 250000;
            *tx_rate = 125;
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
  *tx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default tx sample rate */
  *tx_rate = 625;

  close(sock_client);
  sock_thread[2] = -1;

  return NULL;
}

void *tx_data_handler(void *arg)
{
  int sock_client = sock_thread[3];
  struct pollfd pfd = {sock_client, POLLIN, 0};
  struct timespec timeout;
  useconds_t delay;
  int result, position, limit, offset;
  ssize_t size, rest;
  char buffer[5003];

  memset(tx_data, 0, 8192);

  limit = 512;
  rest = 0;

  while(1)
  {
    delay = *tx_rate * 2;

    timeout.tv_sec = 0;
    timeout.tv_nsec = delay * 1000;

    result = ppoll(&pfd, 1, &timeout, NULL);

    if(result < 0) break;

    /* read ram reader position */
    position = *tx_cntr;

    /* receive 4096 bytes if ready, otherwise sleep */
    if((limit > 0 && position > limit) || (limit == 0 && position < 512))
    {
      offset = limit > 0 ? 0 : 4096;
      limit = limit > 0 ? 0 : 512;

      if(result == 0)
      {
        memset(tx_data + offset, 0, 4096);
        continue;
      }

      if((size = recv(sock_client, buffer, 4096 + rest, 0)) <= 0) break;

      if(size == 4096 + rest)
      {
        memcpy(tx_data + offset, buffer + rest, 4096);
        rest = 0;
      }
      else
      {
        memset(tx_data + offset, 0, 4096);
        rest = abs(rest - size) & 7;
      }
    }
    else
    {
      usleep(delay);
    }
  }

  memset(tx_data, 0, 8192);

  close(sock_client);
  sock_thread[3] = -1;

  return NULL;
}
