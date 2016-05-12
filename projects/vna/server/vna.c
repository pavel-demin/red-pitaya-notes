#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  void *cfg, *sts;
  char *name = "/dev/mem";
  uint32_t *rx_freq, *tx_freq;
  uint32_t *rx_size, *tx_size;
  uint16_t *rx_cntr;
  uint8_t *rst;
  float *rx_data;
  struct sockaddr_in addr;
  uint32_t command, value;
  int64_t start, stop, size, freq;
  int i, j, k, yes = 1;
  float omega, sine, cosine, coeff;
  float re, r0[4], r1[4], r2[4];
  float im, i0[4], i1[4], i2[4];
  float buffer[8];

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
  rx_freq = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  tx_freq = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_cntr = ((uint16_t *)(sts + 12));

  rst = ((uint8_t *)(cfg + 0));
  rx_size = ((uint32_t *)(cfg + 4));
  tx_size = ((uint32_t *)(cfg + 8));

  *rx_size = 250000 - 1;
  *tx_size = 250000 - 1;

  start = 100000;
  stop = 60000000;
  size = 600;

  omega = M_PI / 50.0;
  sine = sin(omega);
  cosine = cos(omega);
  coeff = 2.0 * cosine;

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
          if(value < 5000 || value > 62500000) continue;
          start = value;
          break;
        case 1:
          /* set stop */
          if(value < 5000 || value > 62500000) continue;
          stop = value;
          break;
        case 2:
          /* set size */
          if(value < 1 || value > 16383) continue;
          size = value;
          break;
        case 3:
          /* sweep */
          *rst |= 2; *rst &= ~2;
          *rst &= ~1;
          freq = start;
          for(i = 0; i <= size; ++i)
          {
            if(i > 0) freq = start + (stop - start) * (i - 1) / (size - 1);
            *rx_freq = (uint32_t)floor((freq - 2500) / 125.0e6 * (1<<30) + 0.5);
            *tx_freq = (uint32_t)floor(freq / 125.0e6 * (1<<30) + 0.5);
          }
          *rst |= 1;
          for(i = 0; i <= size; ++i)
          {
            memset(r1, 0, 16);
            memset(i1, 0, 16);
            memset(r2, 0, 16);
            memset(i2, 0, 16);
            while(*rx_cntr < 4000) usleep(200);
            for(j = 0; j < 500; ++j)
            {
              for(k = 0; k < 4; ++k)
              {
                re = *rx_data;
                im = *rx_data;
                r0[k] = coeff * r1[k] - r2[k] + re;
                i0[k] = coeff * i1[k] - i2[k] + im;
                r2[k] = r1[k];
                i2[k] = i1[k];
                r1[k] = r0[k];
                i1[k] = i0[k];
              }
            }
            for(k = 0; k < 4; ++k)
            {
              buffer[2 * k + 0] = (r1[k] - r2[k] * cosine) - (i2[k] * sine);
              buffer[2 * k + 1] = (r2[k] * sine) + (i1[k] - i2[k] * cosine);
            }
            if(i > 0) if(send(sock_client, buffer, 32, MSG_NOSIGNAL) < 0) break;
          }
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
