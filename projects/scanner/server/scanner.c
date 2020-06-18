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

int main()
{
  int fd, sock_server, sock_client;
  struct sockaddr_in addr;
  int i, j, n, counter, position, size, yes = 1;
  uint32_t command, code, data, period, pulses, shdelay, shtime;
  uint32_t *coordinates;
  uint64_t buffer[1024], tmp;
  volatile void *cfg, *sts;
  volatile uint8_t *rst;
  volatile uint16_t *rd_cntr, *wr_cntr;
  volatile uint32_t *dac;
  volatile uint64_t *adc;

  size = 0;
  coordinates = malloc(4194304);

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  dac = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  adc = mmap(NULL, 32*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  wr_cntr = (uint16_t *)(sts + 0);
  rd_cntr = (uint16_t *)(sts + 2);

  rst = (uint8_t *)(cfg + 0);

  /* stop pulse generators */
  *rst &= ~1;

  /* configure trigger edge (0 for negative, 1 for positive) */
  *(uint8_t *)(cfg + 1) = 1;

  /* set trigger mask */
  *(uint8_t *)(cfg + 2) = 1;

  /* set trigger level */
  *(uint8_t *)(cfg + 3) = 1;

  /* set number of ADC samples per pulse */
  *(uint8_t *)(cfg + 4) = 32 - 1;

  /* set number of pulses per pixel */
  pulses = 2;
  *(uint8_t *)(cfg + 5) = pulses - 1;

  period = 25000;

  /* trigger pulse generator */
  /* number of clocks before 1 */
  *(uint32_t *)(cfg + 8) = 0;
  /* number of clocks before 0 */
  *(uint32_t *)(cfg + 12) = 200;
  /* total number of clocks (200e-6 * 125e6) */
  *(uint32_t *)(cfg + 16) = period - 1;

  /* S&H pulse generator */
  /* number of clocks before 1 */
  shdelay = 400;
  *(uint32_t *)(cfg + 20) = shdelay;
  /* number of clocks before 0 */
  shtime = 30;
  *(uint32_t *)(cfg + 24) = shdelay + shtime;
  /* total number of clocks (200e-6 * 143e6) */
  *(uint32_t *)(cfg + 28) = period - 1;

  /* ADC pulse generator */
  /* number of clocks before 1 */
  *(uint32_t *)(cfg + 32) = 2000;
  /* number of clocks before 0 (+1) */
  *(uint32_t *)(cfg + 36) = 2001;
  /* total number of clocks */
  *(uint32_t *)(cfg + 40) = 2001;

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
      if(recv(sock_client, &command, 4, MSG_WAITALL) <= 0) break;
      code = command >> 28;
      data = command & 0xfffffff;
      switch(code)
      {
        case 0:
          /* set trigger period */
          period = data;
          *(uint32_t *)(cfg + 16) = period - 1;
          *(uint32_t *)(cfg + 28) = period - 1;
          break;
        case 1:
          /* set trigger duration */
          *(uint32_t *)(cfg + 12) = data;
          break;
        case 2:
          /* set trigger polarity */
          if(data == 0) *rst &= ~4;
          else if(data == 1) *rst |= 4;
          break;
        case 3:
          /* set S&H delay */
          shdelay = data;
          *(uint32_t *)(cfg + 20) = shdelay;
          *(uint32_t *)(cfg + 24) = shdelay + shtime;
          break;
        case 4:
          /* set S&H duration */
          shtime = data;
          *(uint32_t *)(cfg + 24) = shdelay + shtime;
          break;
        case 5:
          /* set S&H polarity */
          if(data == 0) *rst &= ~8;
          else if(data == 1) *rst |= 8;
          break;
        case 6:
          /* set acquisition delay */
          *(uint32_t *)(cfg + 32) = data;
          *(uint32_t *)(cfg + 36) = data + 1;
          *(uint32_t *)(cfg + 40) = data + 1;
          break;
        case 7:
          /* set number of ADC samples per pulse */
          *(uint8_t *)(cfg + 4) = data - 1;
          break;
        case 8:
          /* set number of pulses per pixel */
          pulses = data;
          *(uint8_t *)(cfg + 5) = pulses - 1;
          break;
        case 9:
          /* clear coordinates */
          size = 0;
          break;
        case 10:
          /* add coordinates */
          if(size >= 1048576) continue;
          coordinates[size] = (data & 0xfffc000) << 2 | (data & 0x3fff);
          ++size;
          break;
        case 11:
          /* start pulse generators */
          *rst |= 1;
          break;
        case 12:
          /* start scanning */
          counter = 0;
          position = 0;
          n = 1024;

          /* stop pulse generators */
          *rst &= ~1;

          /* reset DAC and ADC FIFO */
          *rst |= 2; *rst &= ~2;

          while(counter < size)
          {
            /* read IN1 and IN2 samples from ADC FIFO */
            if(n > size - counter) n = size - counter;
            if(*rd_cntr < n * 2) usleep(500);
            if(*rd_cntr >= n * 2 && counter < size)
            {
              for(j = 0; j < n; ++j) buffer[j] = *adc;
              if(send(sock_client, buffer, n * 8, MSG_NOSIGNAL) < 0) break;
              counter += n;
            }

            /* write OUT1 and OUT2 samples to DAC FIFO */
            while(*wr_cntr < 16384 - pulses && position < size)
            {
              for(j = 0; j < pulses; ++j) *dac = coordinates[position];
              ++position;
            }

            /* start pulse generators */
            *rst |= 1;
          }

          break;
      }
    }

    /* stop pulse generators */
    *rst &= ~1;

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
