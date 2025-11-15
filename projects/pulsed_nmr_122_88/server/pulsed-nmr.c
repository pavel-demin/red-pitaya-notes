#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_DAC 0x0F /* AD5622 address 3 */

ssize_t i2c_write_data16(int fd, uint16_t data)
{
  uint8_t buffer[2];
  buffer[0] = data >> 8;
  buffer[1] = data;
  return write(fd, buffer, 2);
}

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  int i2c_fd, i2c_dac;
  volatile void *cfg, *sts, *rx_data_fifo, *tx_evts_fifo, *rx_evts_fifo;
  volatile uint32_t *rx_freq, *tx_freq;
  volatile uint16_t *rx_rate, *rx_data_cntr, *tx_evts_cntr, *rx_evts_cntr;
  volatile int16_t *out2_level;
  volatile uint8_t *rst, *pins;
  struct sockaddr_in addr;
  uint64_t code, command, counter, data, *tx_evts, *rx_evts;
  uint32_t tx_evts_pos, tx_evts_len, rx_evts_pos, rx_evts_len, *buffer;
  uint32_t timeout = 1000, yes = 1;
  size_t n;

  tx_evts_len = 0;
  tx_evts = malloc(16777216);
  rx_evts_len = 0;
  rx_evts = malloc(8388608);

  buffer = malloc(32768);

  if(tx_evts == NULL || buffer == NULL)
  {
    perror("malloc");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  i2c_dac = 0;
  if((i2c_fd = open("/dev/i2c-0", O_RDWR)) >= 0)
  {
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC) >= 0)
    {
      if(i2c_write_data16(i2c_fd, 0x0000) > 0)
      {
        i2c_dac = 1;
      }
    }
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  tx_evts_fifo = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
  rx_evts_fifo = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x43000000);
  rx_data_fifo = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x44000000);

  rst = (uint8_t *)(cfg + 0);
  pins = (uint8_t *)(cfg + 1);

  tx_freq = (uint32_t *)(cfg + 4);

  rx_freq = (uint32_t *)(cfg + 8);
  rx_rate = (uint16_t *)(cfg + 12);

  out2_level = (int16_t *)(cfg + 14);

  tx_evts_cntr = (uint16_t *)(sts + 0);
  rx_evts_cntr = (uint16_t *)(sts + 2);
  rx_data_cntr = (uint16_t *)(sts + 4);

  *rst &= ~7;

  /* set default TX phase increment */
  *tx_freq = (uint32_t)floor(10000000 / 122.88e6 * 0x3fffffff + 0.5);

  /* set default RX phase increment */
  *rx_freq = (uint32_t)floor(10000000 / 122.88e6 * 0x3fffffff + 0.5);
  /* set default RX sample rate */
  *rx_rate = 48;

  /* set default OUT2 level */
  *out2_level = 0;

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  setsockopt(sock_server, SOL_SOCKET, SO_REUSEADDR, &yes , sizeof(yes));

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

    setsockopt(sock_client, SOL_TCP, TCP_USER_TIMEOUT, &timeout, sizeof(timeout));

    while(1)
    {
      if(recv(sock_client, (char *)&command, 8, MSG_WAITALL) <= 0) break;
      code = command >> 60;
      data = command & 0xfffffffffffffffULL;
      switch(code)
      {
        case 0:
          /* set TX and RX phase increments */
          *tx_freq = (uint32_t)floor((data & 0x3fffffff) / 122.88e6 * 0x3fffffff + 0.5);
          *rx_freq = (uint32_t)floor((data >> 30) / 122.88e6 * 0x3fffffff + 0.5);
          break;
        case 1:
          /* set RX sample rate */
          if(data < 48 || data > 3072) continue;
          *rx_rate = data;
          break;
        case 2:
          /* set DAC level */
          if(data > 4095) continue;
          if(i2c_dac == 0) continue;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC);
          i2c_write_data16(i2c_fd, data);
          break;
        case 3:
          /* set OUT2 level */
          if(data > 32766) continue;
          *out2_level = data;
          break;
        case 4:
          /* set pin */
          if(data > 6) continue;
          *pins |= (1 << data);
          break;
        case 5:
          /* clear pin */
          if(data > 6) continue;
          *pins &= ~(1 << data);
          break;
        case 6:
          /* clear TX and RX events */
          tx_evts_len = 0;
          rx_evts_len = 0;
          break;
        case 7:
          /* add TX event */
          if(tx_evts_len >= 1048576) continue;
          ++tx_evts_len;
          /* set delay, sync, gate and level */
          memcpy(tx_evts + (tx_evts_len - 1) * 2, &data, 8);
          memset(tx_evts + (tx_evts_len - 1) * 2 + 1, 0, 8);
          break;
        case 8:
          /* set TX and RX phases */
          memcpy(tx_evts + (tx_evts_len - 1) * 2 + 1, &data, 8);
          break;
        case 9:
          /* add RX event */
          if(rx_evts_len >= 1048576) continue;
          ++rx_evts_len;
          memcpy(rx_evts + (rx_evts_len - 1), &data, 8);
          break;
        case 10:
          /* start sequence */
          counter = 0;
          tx_evts_pos = 0;
          rx_evts_pos = 0;
          n = 2048;

          /* stop TX and RX */
          *rst &= ~1;

          /* clear TX and RX FIFO */
          *rst &= ~6; *rst |= 6;

          while(counter < data)
          {
            /* read I/Q samples from FIFO */
            if(n > data - counter) n = data - counter;
            if(*rx_data_cntr < n * 4) usleep(500);
            if(*rx_data_cntr >= n * 4)
            {
              memcpy(buffer, rx_data_fifo, n * 16);
              if(send(sock_client, buffer, n * 16, MSG_NOSIGNAL) < 0) break;
              counter += n;
            }

            /* write TX events to FIFO */
            while(*tx_evts_cntr < 16380 && tx_evts_pos < tx_evts_len)
            {
              memcpy(tx_evts_fifo, tx_evts + tx_evts_pos * 2, 16);
              ++tx_evts_pos;
            }

            /* write RX events to FIFO */
            while(*rx_evts_cntr < 8190 && rx_evts_pos < rx_evts_len)
            {
              memcpy(rx_evts_fifo, rx_evts + rx_evts_pos, 8);
              ++rx_evts_pos;
            }

            /* start RX and TX */
            *rst |= 1;
          }

          *rst &= ~7;
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
