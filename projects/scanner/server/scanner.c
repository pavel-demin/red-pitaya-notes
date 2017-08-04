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
  int i, j, counter, yes = 1;
  uint32_t command, code, data, period, pulses, shdelay, shtime;
  uint64_t buffer[1024], tmp;
  volatile uint32_t *slcr;
  volatile void *cfg, *sts;
  volatile uint8_t *rst;
  volatile uint32_t *dac;
  volatile uint64_t *adc;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  slcr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0xF8000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  dac = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  adc = mmap(NULL, 32*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rst = cfg;

  /* stop pulse generators */
  *rst &= ~1;

  /* set number of ADC samples per pulse */
  *(uint8_t *)(cfg + 2) = 25;

  /* set number of pulses per pixel */
  pulses = 1;
  *(uint8_t *)(cfg + 3) = pulses - 1;

  period = 29000;

  /* trigger pulse generator */
  /* number of clocks before 1 */
  *(uint32_t *)(cfg + 4) = 0;
  /* number of clocks before 0 */
  *(uint32_t *)(cfg + 8) = 200;
  /* total number of clocks (200e-6 * 143e6) */
  *(uint32_t *)(cfg + 12) = period - 1;

  /* S&H pulse generator */
  /* number of clocks before 1 */
  shdelay = 400;
  *(uint32_t *)(cfg + 16) = shdelay;
  /* number of clocks before 0 */
  shtime = 30;
  *(uint32_t *)(cfg + 20) = shdelay + shtime;
  /* total number of clocks (200e-6 * 143e6) */
  *(uint32_t *)(cfg + 24) = period - 1;

  /* DAC pulse generator */
  /* number of clocks before 1 */
  *(uint32_t *)(cfg + 28) = 0;
  /* number of clocks before 0 (+1) */
  *(uint32_t *)(cfg + 32) = 1;
  /* total number of clocks (200e-6 * 143e6) */
  *(uint32_t *)(cfg + 36) = pulses * period - 1;

  /* ADC pulse generator */
  /* number of clocks before 1 */
  *(uint32_t *)(cfg + 40) = 2000;
  /* number of clocks before 0 (+1) */
  *(uint32_t *)(cfg + 44) = 2001;
  /* total number of clocks */
  *(uint32_t *)(cfg + 48) = period - 1;

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
          *(uint32_t *)(cfg + 12) = period - 1;
          *(uint32_t *)(cfg + 24) = period - 1;
          *(uint32_t *)(cfg + 36) = pulses * period - 1;
          *(uint32_t *)(cfg + 48) = period - 1;
          break;
        case 1:
          /* set trigger duration */
          *(uint32_t *)(cfg + 8) = data;
          break;
        case 2:
          /* set trigger polarity */
          if(data == 0) *rst &= ~4;
          else if(data == 1) *rst |= 4;
          break;
        case 3:
          /* set S&H delay */
          shdelay = data;
          *(uint32_t *)(cfg + 16) = shdelay;
          *(uint32_t *)(cfg + 20) = shdelay + shtime;
          break;
        case 4:
          /* set S&H duration */
          shtime = data;
          *(uint32_t *)(cfg + 20) = shdelay + shtime;
          break;
        case 5:
          /* set S&H polarity */
          if(data == 0) *rst &= ~8;
          else if(data == 1) *rst |= 8;
          break;
        case 6:
          /* set acquisition delay */
          *(uint32_t *)(cfg + 40) = data;
          *(uint32_t *)(cfg + 44) = data + 1;
          break;
        case 7:
          /* set number of ADC samples per pulse */
          *(uint8_t *)(cfg + 2) = data - 1;
          break;
        case 8:
          /* set number of pulses per pixel */
          pulses = data;
          *(uint8_t *)(cfg + 3) = pulses - 1;
          *(uint32_t *)(cfg + 36) = pulses * period - 1;
          break;
        case 9:
          /* start pulse generators */
          *rst |= 1;
          break;
        case 10:
          /* start scanning */
          counter = 0;

          /* stop pulse generators */
          *rst &= ~1;

          /* reset DAC and ADC FIFO */
          *rst |= 2;
          *rst &= ~2;

          /* write OUT1 and OUT2 samples to DAC FIFO */
          /* read IN1 and IN2 samples from ADC FIFO */
          for(i = 0; i <= 8176;)
          {
            if(*(uint16_t *)(sts + 2) >= 2048 && counter < 256)
            {
              for(j = 0; j < 1024; ++j) buffer[j] = *adc;
              for(j = 512; j < 768; ++j)
              {
                tmp = buffer[j];
                buffer[j] = buffer[1535 - j];
                buffer[1535 - j] = tmp;
              }
              if(send(sock_client, buffer, 8192, MSG_NOSIGNAL) < 0) break;
              ++counter;
            }

            /* wait if there is not enough free space in DAC FIFO */
            if(*(uint16_t *)(sts + 0) > 15000)
            {
              if(*(uint16_t *)(sts + 2) < 2048) usleep(500);
              continue;
            }

            for(j = 0; j <= 8176; j += 16)
            {
              *dac = (i << 16) | j;
            }

            i += 16;

            for(j = 8176; j >= 0; j -= 16)
            {
              *dac = (i << 16) | j;
            }

            i += 16;

            if(i == 32)
            {
              /* start pulse generators */
              *rst |= 1;
            }
          }

          /* read remaining IN1 and IN2 samples from ADC FIFO */
          while(counter < 256)
          {
            if(*(uint16_t *)(sts + 2) >= 2048)
            {
              for(j = 0; j < 1024; ++j) buffer[j] = *adc;
              for(j = 512; j < 768; ++j)
              {
                tmp = buffer[j];
                buffer[j] = buffer[1535 - j];
                buffer[1535 - j] = tmp;
              }
              if(send(sock_client, buffer, 8192, MSG_NOSIGNAL) < 0) break;
              ++counter;
            }
            usleep(500);
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
