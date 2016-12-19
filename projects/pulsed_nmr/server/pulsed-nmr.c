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
  volatile uint32_t *slcr, *rx_freq, *rx_rate;
  volatile uint16_t *rx_cntr, *tx_size;
  volatile uint8_t *rx_rst, *tx_rst;
  volatile uint64_t *rx_data;
  void *tx_data;
  float tx_freq;
  struct sockaddr_in addr;
  uint32_t command, value;
  int16_t pulse[32768];
  uint64_t buffer[8192];
  int i, j, size, yes = 1;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  tx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_rst = ((uint8_t *)(cfg + 0));
  rx_freq = ((uint32_t *)(cfg + 4));
  rx_rate = ((uint32_t *)(cfg + 8));
  rx_cntr = ((uint16_t *)(sts + 0));

  tx_rst = ((uint8_t *)(cfg + 1));
  tx_size = ((uint16_t *)(cfg + 12));

  /* set FPGA clock to 143 MHz */
  slcr[2] = 0xDF0D;
  slcr[92] = (slcr[92] & ~0x03F03F30) | 0x00100700;

  /* set default rx phase increment */
  *rx_freq = (uint32_t)floor(19000000 / 125.0e6 * (1<<30) + 0.5);
  /* set default rx sample rate */
  *rx_rate = 250;

  /* fill tx buffer with zeros */
  memset(tx_data, 0, 65536);

  /* local oscillator for the excitation pulse */
  tx_freq = 19.0e6;
  for(i = 0; i < 32768; ++i)
  {
    pulse[i] = (int16_t)floor(8000.0 * sin(i * 2.0 * M_PI * tx_freq / 125.0e6) + 0.5);
  }

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
          /* set rx phase increment */
          if(value < 0 || value > 60000000) continue;
          *rx_freq = (uint32_t)floor(value / 125.0e6 * (1<<30) + 0.5);
          break;
        case 1:
          /* set rx sample rate */
          switch(command & 7)
          {
            case 0:
              *rx_rate = 2500;
              break;
            case 1:
              *rx_rate = 1250;
              break;
            case 2:
              *rx_rate = 250;
              break;
            case 3:
              *rx_rate = 125;
              break;
            case 4:
              *rx_rate = 25;
              break;
          }
          break;
        case 2:
          /* set A width */
          size = (uint32_t)floor(125.0e6 * value / (10.0 * tx_freq) + 0.5);
          if(size > 32768) continue;
          *tx_size = size - 1;
          memset(tx_data, 0, 65536);
          memcpy(tx_data, pulse, 2 * size);
          break;
        case 3:
          /* fire */
          *rx_rst |= 1; *rx_rst &= ~1;
          *tx_rst &= ~1; *tx_rst |= 1;
          /* transfer 10 * 5k = 50k samples */
          for(i = 0; i < 10; ++i)
          {
            while(*rx_cntr < 10000) usleep(500);
            for(j = 0; j < 5000; ++j) buffer[j] = *rx_data;
            send(sock_client, buffer, 40000, MSG_NOSIGNAL);
          }
          break;
      }
    }

    /* set default rx phase increment */
    *rx_freq = (uint32_t)floor(19000000 / 125.0e6 * (1<<30) + 0.5);
    /* set default rx sample rate */
    *rx_rate = 250;

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
