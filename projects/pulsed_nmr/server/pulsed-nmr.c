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
  volatile uint32_t *tx_data, *rx_freq, *tx_freq;
  volatile uint16_t *rx_rate, *rx_cntr, *tx_cntr;
  volatile int16_t *tx_level;
  volatile uint8_t *rx_rst, *tx_rst;
  volatile uint64_t *rx_data;
  struct sockaddr_in addr;
  uint32_t command, code, data;
  uint32_t *pulses;
  uint64_t *buffer;
  int i, n, counter, position, size, yes = 1;

  size = 0;
  pulses = malloc(16777216);
  buffer = malloc(32768);

  if(pulses == NULL || buffer == NULL)
  {
    perror("malloc");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  tx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_rst = ((uint8_t *)(cfg + 0));
  rx_freq = ((uint32_t *)(cfg + 4));
  rx_rate = ((uint16_t *)(cfg + 8));
  tx_level = ((int16_t *)(cfg + 10));
  rx_cntr = ((uint16_t *)(sts + 12));

  tx_rst = ((uint8_t *)(cfg + 1));
  tx_freq = ((uint32_t *)(cfg + 12));
  tx_cntr = ((uint16_t *)(sts + 14));

  *rx_rst |= 1;
  *rx_rst &= ~2;
  *tx_rst |= 1;

  /* set default RX phase increment */
  *rx_freq = (uint32_t)floor(19000000 / 125.0e6 * (1<<30) + 0.5);
  /* set default RX sample rate */
  *rx_rate = 250;

  /* set default TX level */
  *tx_level = 0;

  /* set default TX phase increment */
  *tx_freq = (uint32_t)floor(19000000 / 125.0e6 * (1<<30) + 0.5);

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
      code = command >> 28;
      data = command & 0xfffffff;
      switch(code)
      {
        case 0:
          /* set RX phase increment */
          if(data > 62500000) continue;
          *rx_freq = (uint32_t)floor(data / 125.0e6 * (1<<30) + 0.5);
          break;
        case 1:
          /* set TX phase increment */
          if(data > 62500000) continue;
          *tx_freq = (uint32_t)floor(data / 125.0e6 * (1<<30) + 0.5);
          break;
        case 2:
          /* set RX sample rate */
          if(data < 50 || data > 2500) continue;
          *rx_rate = data;
          break;
        case 3:
          /* set TX level */
          if(data > 32766) continue;
          *tx_level = data;
          break;
        case 4:
          /* clear pulses */
          size = 0;
          break;
        case 5:
          /* add pulse */
          if(size >= 1048576) continue;
          ++size;
          memset(pulses + (size - 1) * 16, 0, 16);
          break;
        case 6:
          /* set pulse level */
          if(data > 32766) continue;
          pulses[(size - 1) * 4 + 0] = data;
          break;
        case 7:
          /* set pulse phase */
          if(data > 359) continue;
          pulses[(size - 1) * 4 + 1] = (uint32_t)floor(data / 360.0 * (1<<30) + 0.5);
          break;
        case 8:
          /* set pulse delay */
          pulses[(size - 1) * 4 + 2] = data;
          break;
        case 9:
          /* set pulse width */
          pulses[(size - 1) * 4 + 3] = data;
          break;
        case 10:
          /* start sequence */
          counter = 0;
          position = 0;
          n = 2048;

          /* stop RX and TX */
          *rx_rst &= ~2;

          /* clear RX FIFO */
          *rx_rst |= 1; *rx_rst &= ~1;

          /* clear TX FIFO */
          *tx_rst |= 1; *tx_rst &= ~1;

          while(counter < data)
          {
            /* read I/Q samples from RX FIFO */
            if(counter + n > data) n = data - counter;
            if(*rx_cntr >= n * 4)
            {
              for(i = 0; i < n * 2; ++i) buffer[i] = *rx_data;
              if(send(sock_client, buffer, n * 16, MSG_NOSIGNAL) < 0) break;
              counter += n;
            }

            /* write pulses to TX FIFO */
            while(*tx_cntr < 16384 && position < size * 4)
            {
              *tx_data = pulses[position];
              ++position;
            }

            /* start RX and TX */
            *rx_rst |= 2;

            if(*rx_cntr < n * 4) usleep(500);
          }

          *rx_rst |= 1;
          *rx_rst &= ~2;
          *tx_rst |= 1;
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
