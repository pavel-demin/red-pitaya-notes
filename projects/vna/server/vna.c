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
  uint64_t *rx_data;
  struct sockaddr_in addr;
  uint32_t command, value, freq;
  uint64_t buffer[4000];
  int i, j, start, step, size, yes = 1;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_freq = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  tx_freq = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);
  rx_data = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);

  rx_cntr = ((uint16_t *)(sts + 12));

  rst = ((uint8_t *)(cfg + 0));
  rx_size = ((uint32_t *)(cfg + 4));
  tx_size = ((uint32_t *)(cfg + 8));

  *rx_size = 625000;
  *tx_size = 625000;

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
          if(value < 100 || value > 60000) continue;
          start = value;
          break;
        case 1:
          /* set step */
          if(value < 100 || value > 10000) continue;
          step = value;
          break;
        case 2:
          /* set size */
          if(value < 10 || value > 1000) continue;
          size = value;
          break;
        case 3:
          /* sweep */
          *rst |= 2; *rst &= ~2;
          *rst &= ~1;
          freq = start;
          for(i = 0; i <= size; ++i)
          {
            *rx_freq = (uint32_t)floor((freq - 1) / 125.0e3 * (1<<30) + 0.5);
            *tx_freq = (uint32_t)floor(freq / 125.0e3 * (1<<30) + 0.5);
            if(i > 0) freq += step;
          }
          *rst |= 1;
          for(i = 0; i <= size; ++i)
          {
            while(*rx_cntr < 8000) usleep(20000);
            for(j = 0; j < 4000; ++j) buffer[j] = *rx_data;
            if(i > 0) send(sock_client, buffer, 32000, MSG_NOSIGNAL);
          }
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
