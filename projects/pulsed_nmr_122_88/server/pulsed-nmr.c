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
  void *cfg, *sts;
  volatile uint32_t *fifo, *rx_freq, *tx_freq;
  volatile uint16_t *rx_rate, *rx_cntr, *tx_cntr;
  volatile int16_t *tx_level;
  volatile uint8_t *rx_rst, *tx_rst;
  struct sockaddr_in addr;
  uint64_t command, code, data, counter;
  uint32_t *buffer, *pulses;
  int i, n, position, size, yes = 1;

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
  fifo = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_freq = (uint32_t *)(cfg + 4);
  rx_rate = (uint16_t *)(cfg + 8);
  tx_level = (int16_t *)(cfg + 10);
  rx_cntr = (uint16_t *)(sts + 0);

  tx_rst = (uint8_t *)(cfg + 1);
  tx_freq = (uint32_t *)(cfg + 12);
  tx_cntr = (uint16_t *)(sts + 2);

  *rx_rst &= ~1;
  *rx_rst &= ~2;
  *tx_rst &= ~1;

  /* set default RX phase increment */
  *rx_freq = (uint32_t)floor(19000000 / 122.88e6 * (1<<30) + 0.5);
  /* set default RX sample rate */
  *rx_rate = 384;

  /* set default TX level */
  *tx_level = 0;

  /* set default TX phase increment */
  *tx_freq = (uint32_t)floor(19000000 / 122.88e6 * (1<<30) + 0.5);

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
      if(recv(sock_client, (char *)&command, 8, MSG_WAITALL) <= 0) break;
      code = command >> 60;
      data = command & 0xfffffffffffffffULL;
      switch(code)
      {
        case 0:
          /* set RX phase increment */
          if(data > 61440000) continue;
          *rx_freq = (uint32_t)floor(data / 122.88e6 * (1<<30) + 0.5);
          break;
        case 1:
          /* set TX phase increment */
          if(data > 61440000) continue;
          *tx_freq = (uint32_t)floor(data / 122.88e6 * (1<<30) + 0.5);
          break;
        case 2:
          /* set RX sample rate */
          if(data < 48 || data > 3072) continue;
          *rx_rate = data;
          break;
        case 3:
          /* set TX level */
          if(data > 32766) continue;
          *tx_level = data;
          break;
        case 4:
          /* set pin */
          if(data < 1 || data > 7) continue;
          *tx_rst |= (1 << data);
          break;
        case 5:
          /* clear pin */
          if(data < 1 || data > 7) continue;
          *tx_rst &= ~(1 << data);
          break;
        case 6:
          /* set DAC */
          if(i2c_dac == 0) continue;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC);
          i2c_write_data16(i2c_fd, data);
          break;
        case 7:
          /* clear pulses */
          size = 0;
          break;
        case 8:
          /* add pulse */
          if(size >= 1048576) continue;
          ++size;
          memset(pulses + (size - 1) * 4, 0, 16);
          /* set pulse width */
          memcpy(pulses + (size - 1) * 4, &data, 8);
          break;
        case 9:
          /* set pulse phase and level */
          memcpy(pulses + (size - 1) * 4 + 2, &data, 8);
          break;
        case 10:
          /* start sequence */
          counter = 0;
          position = 0;
          n = 2048;

          /* stop RX and TX */
          *rx_rst &= ~2;

          /* clear RX FIFO */
          *rx_rst &= ~1; *rx_rst |= 1;

          /* clear TX FIFO */
          *tx_rst &= ~1; *tx_rst |= 1;

          while(counter < data)
          {
            /* read I/Q samples from RX FIFO */
            if(n > data - counter) n = data - counter;
            if(*rx_cntr < n * 4) usleep(500);
            if(*rx_cntr >= n * 4)
            {
              memcpy(buffer, fifo, n * 16);
              if(send(sock_client, buffer, n * 16, MSG_NOSIGNAL) < 0) break;
              counter += n;
            }

            /* write pulses to TX FIFO */
            while(*tx_cntr < 16384 && position < size * 4)
            {
              *fifo = pulses[position];
              ++position;
            }

            /* start RX and TX */
            *rx_rst |= 2;
          }

          *rx_rst &= ~1;
          *rx_rst &= ~2;
          *tx_rst &= ~1;
          break;
      }
    }

    close(sock_client);
  }

  close(sock_server);

  return EXIT_SUCCESS;
}
