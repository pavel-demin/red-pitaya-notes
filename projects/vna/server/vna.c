#include <stdio.h>
#include <stdlib.h>
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

#include "jack/ringbuffer.c"

volatile uint16_t *rx_cntr[3];
volatile uint64_t *rx_data[3];

int sock_thread = -1;
uint32_t rate_thread = 1;
uint32_t size_thread = 600;

jack_ringbuffer_t *sweep_data = 0;

void *read_handler(void *arg);
void *send_handler(void *arg);

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  pthread_t thread;
  volatile void *cfg, *sts;
  char *name = "/dev/mem";
  volatile uint32_t *rx_freq, *tx_freq;
  volatile uint32_t *rx_size, *tx_size;
  volatile int16_t *tx_level;
  volatile uint8_t *rst;
  struct sockaddr_in addr;
  uint32_t command, rate;
  int32_t value, corr;
  int64_t start, stop, size, freq;
  int i, yes = 1;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data[0] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rx_data[1] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
  rx_data[2] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
  rx_freq = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  tx_freq = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_cntr[0] = ((uint16_t *)(sts + 12));
  rx_cntr[1] = ((uint16_t *)(sts + 14));
  rx_cntr[2] = ((uint16_t *)(sts + 16));

  rst = ((uint8_t *)(cfg + 0));
  rx_size = ((uint32_t *)(cfg + 4));
  tx_size = ((uint32_t *)(cfg + 8));
  tx_level = ((int16_t *)(cfg + 12));

  *rx_size = 250000 - 1;
  *tx_size = 250000 - 1;
  *tx_level = 32767;

  start = 100000;
  stop = 60000000;
  size = 600;
  rate = 1;
  corr = 0;

  sweep_data = jack_ringbuffer_create(65536);

  *rst &= ~3;
  *rst |= 4;

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

    while(1)
    {
      if(recv(sock_client, (char *)&command, 4, MSG_WAITALL) <= 0) break;
      value = command & 0xfffffff;
      switch(command >> 28)
      {
        case 0:
          /* set start */
          if(value < 0 || value > 62000000) continue;
          start = value;
          break;
        case 1:
          /* set stop */
          if(value < 0 || value > 62000000) continue;
          stop = value;
          break;
        case 2:
          /* set size */
          if(value < 1 || value > 16383) continue;
          size = value;
          break;
        case 3:
          /* set rate */
          if(value < 1 || value > 10000) continue;
          rate = value;
          *rx_size = 250000 * rate - 1;
          *tx_size = 250000 * rate - 1;
          break;
        case 4:
          /* set correction */
          if(value < -100000 || value > 100000) continue;
          corr = value;
          break;
        case 5:
          /* set level */
          if(value < -32767 || value > 32767) continue;
          *tx_level = value;
          break;
        case 6:
          /* sweep */
          *rst &= ~3;
          *rst |= 4;
          *rst &= ~4;
          *rst |= 2;
          rate_thread = rate;
          size_thread = size;
          sock_thread = sock_client;
          jack_ringbuffer_reset(sweep_data);
          if(pthread_create(&thread, NULL, read_handler, NULL) < 0)
          {
            perror("pthread_create");
            return EXIT_FAILURE;
          }
          pthread_detach(thread);
          if(pthread_create(&thread, NULL, send_handler, NULL) < 0)
          {
            perror("pthread_create");
            return EXIT_FAILURE;
          }
          pthread_detach(thread);
          freq = start;
          for(i = 0; i <= size; ++i)
          {
            if(i > 0) freq = start + (stop - start) * (i - 1) / (size - 1);
            freq *= (1.0 + 1.0e-9 * corr);
            *rx_freq = (uint32_t)floor((freq + 2500) / 125.0e6 * (1<<30) + 0.5);
            *tx_freq = (uint32_t)floor(freq / 125.0e6 * (1<<30) + 0.5);
          }
          *rst |= 1;
          break;
        case 7:
          /* cancel */
          *rst &= ~3;
          *rst |= 4;
          sock_thread = -1;
          break;
      }
    }

    *rst &= ~3;
    *rst |= 4;
    sock_thread = -1;
    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}

void *read_handler(void *arg)
{
  int i, cntr;
  uint32_t rate = rate_thread;
  uint32_t size = size_thread;
  uint64_t buffer[3];

  cntr = 0;
  while(cntr < (size + 1) * rate)
  {
    if(sock_thread < 0) break;

    if(*rx_cntr[0] < 1000)
    {
      usleep(100);
      continue;
    }

    for(i = 0; i < 500; ++i)
    {
      buffer[0] = *rx_data[0];
      buffer[1] = *rx_data[1];
      buffer[2] = *rx_data[2];
      jack_ringbuffer_write(sweep_data, (char *)buffer, 24);
    }

    ++cntr;
  }

  return NULL;
}

void *send_handler(void *arg)
{
  int i, j, k, cntr;
  uint32_t rate = rate_thread;
  uint32_t size = size_thread;
  float omega, sine, cosine, coeff;
  float re, r0[3], r1[3], r2[3];
  float im, i0[3], i1[3], i2[3];
  float *w, buffer[6];

  w = malloc(4 * 500 * rate);

  omega = -M_PI / 50.0;
  sine = sin(omega);
  cosine = cos(omega);
  coeff = 2.0 * cosine;

  memset(r1, 0, 12);
  memset(i1, 0, 12);
  memset(r2, 0, 12);
  memset(i2, 0, 12);

  k = 0;
  cntr = 0;
  while(cntr < (size + 1) * rate)
  {
    if(sock_thread < 0) break;

    if(jack_ringbuffer_read_space(sweep_data) < 12000)
    {
      usleep(100);
      continue;
    }

    for(i = 0; i < 500; ++i)
    {
      if(cntr < rate) w[k] = sin(M_PI * k / (500 * rate - 1));
      for(j = 0; j < 3; ++j)
      {
        jack_ringbuffer_read(sweep_data, (char *)&re, 4);
        jack_ringbuffer_read(sweep_data, (char *)&im, 4);
        r0[j] = coeff * r1[j] - r2[j] + re * w[k];
        i0[j] = coeff * i1[j] - i2[j] + im * w[k];
        r2[j] = r1[j];
        i2[j] = i1[j];
        r1[j] = r0[j];
        i1[j] = i0[j];
      }
      ++k;
    }

    ++cntr;

    if(cntr % rate) continue;

    for(j = 0; j < 3; ++j)
    {
      buffer[2 * j + 0] = (r1[j] - r2[j] * cosine) - (i2[j] * sine);
      buffer[2 * j + 1] = (r2[j] * sine) + (i1[j] - i2[j] * cosine);
    }

    memset(r1, 0, 12);
    memset(i1, 0, 12);
    memset(r2, 0, 12);
    memset(i2, 0, 12);

    k = 0;

    if(send(sock_thread, buffer, 24, MSG_NOSIGNAL) < 0) break;
  }

  free(w);

  return NULL;
}
