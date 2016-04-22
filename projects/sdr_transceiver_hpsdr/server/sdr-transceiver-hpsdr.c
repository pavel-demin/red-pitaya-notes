/* 19.04.2016 DC2PD : add code for bandpass and antenna switching via I2C. */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_PENE 0x20 /* PCA9555 address 0 */
#define ADDR_ALEX 0x21 /* PCA9555 address 1 */

uint32_t *rx_freq[4], *rx_rate, *tx_freq;
uint16_t *rx_cntr[4], *tx_cntr;
uint8_t *gpio_in, *gpio_out, *rx_rst, *tx_rst;
uint64_t *rx_data[4];
void *tx_data;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int receivers = 1;

int sock_ep2;
struct sockaddr_in addr_ep6;

int enable_thread = 0;
int active_thread = 0;

int vna = 0;

void process_ep2(uint8_t *frame);
void *handler_ep6(void *arg);

/* variables to handle PCA9555 board */
int i2c_fd;
int i2c_pene = 0;
int i2c_alex = 0;

ssize_t i2c_write(int fd, uint8_t addr, uint16_t data)
{
  uint8_t buffer[3];
  buffer[0] = addr;
  buffer[1] = data;
  buffer[2] = data >> 8;
  return write(fd, buffer, 3);
}

int main(int argc, char *argv[])
{
  int fd, i;
  ssize_t size;
  pthread_t thread;
  void *cfg, *sts;
  char *name = "/dev/mem";
  uint8_t buffer[1032];
  uint8_t reply[11] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 21, 0};
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from;
  socklen_t size_from;
  int yes = 1;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  if((i2c_fd = open("/dev/i2c-0", O_RDWR)) >= 0)
  {
    if(ioctl(i2c_fd, I2C_SLAVE_FORCE, ADDR_PENE) >= 0)
    {
      /* set all pins to low */
      if(i2c_write(i2c_fd, 0x02, 0x0000))
      {
        i2c_pene = 1;
        /* configure all pins as output */
        i2c_write(i2c_fd, 0x06, 0x0000);
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_ALEX) >= 0)
    {
      /* set all pins to low */
      if(i2c_write(i2c_fd, 0x02, 0x0000))
      {
        i2c_alex = 1;
        /* configure all pins as output */
        i2c_write(i2c_fd, 0x06, 0x0000);
      }
    }
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data[0] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rx_data[1] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
  rx_data[2] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
  rx_data[3] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
  tx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);

  *(uint32_t *)(tx_data + 8) = 165;

  rx_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  gpio_out = ((uint8_t *)(cfg + 2));

  rx_rate = ((uint32_t *)(cfg + 4));

  rx_freq[0] = ((uint32_t *)(cfg + 8));
  rx_cntr[0] = ((uint16_t *)(sts + 12));

  rx_freq[1] = ((uint32_t *)(cfg + 12));
  rx_cntr[1] = ((uint16_t *)(sts + 14));

  rx_freq[2] = ((uint32_t *)(cfg + 16));
  rx_cntr[2] = ((uint16_t *)(sts + 16));

  rx_freq[3] = ((uint32_t *)(cfg + 20));
  rx_cntr[3] = ((uint16_t *)(sts + 18));

  tx_freq = ((uint32_t *)(cfg + 32));
  tx_cntr = ((uint16_t *)(sts + 20));

  gpio_in = ((uint8_t *)(sts + 22));

  /* set I/Q data for the VNA mode */
  *((uint64_t *)(cfg + 24)) = 2000000;
  *tx_rst &= ~2;

  /* set all GPIO pins to low */
  *gpio_out = 0;

  /* set default rx phase increment */
  *rx_freq[0] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[1] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[2] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[3] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate = 1000;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);

  *tx_rst |= 1;
  *tx_rst &= ~1;

  if((sock_ep2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  strncpy(hwaddr.ifr_name, "eth0", IFNAMSIZ);
  ioctl(sock_ep2, SIOCGIFHWADDR, &hwaddr);
  for(i = 0; i < 6; ++i) reply[i + 3] = hwaddr.ifr_addr.sa_data[i];

  setsockopt(sock_ep2, SOL_SOCKET, SO_REUSEADDR, (void *)&yes , sizeof(yes));

  memset(&addr_ep2, 0, sizeof(addr_ep2));
  addr_ep2.sin_family = AF_INET;
  addr_ep2.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_ep2.sin_port = htons(1024);

  if(bind(sock_ep2, (struct sockaddr *)&addr_ep2, sizeof(addr_ep2)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  while(1)
  {
    size_from = sizeof(addr_from);
    size = recvfrom(sock_ep2, buffer, 1032, 0, (struct sockaddr *)&addr_from, &size_from);
    if(size < 0)
    {
      perror("recvfrom");
      return EXIT_FAILURE;
    }

    switch(*(uint32_t *)buffer)
    {
      case 0x0201feef:
        while(*tx_cntr > 16258) usleep(1000);
        if(*tx_cntr == 0) memset(tx_data, 0, 65032);
        if((*gpio_out & 1) | (*gpio_in & 1))
        {
          for(i = 0; i < 504; i += 8) memcpy(tx_data, buffer + 20 + i, 4);
          for(i = 0; i < 504; i += 8) memcpy(tx_data, buffer + 532 + i, 4);
        }
        else
        {
          memset(tx_data, 0, 504);
        }
        process_ep2(buffer + 11);
        process_ep2(buffer + 523);
        break;
      case 0x0002feef:
        reply[2] = 2 + active_thread;
        memset(buffer, 0, 60);
        memcpy(buffer, reply, 11);
        sendto(sock_ep2, buffer, 60, 0, (struct sockaddr *)&addr_from, size_from);
        break;
      case 0x0004feef:
        enable_thread = 0;
        while(active_thread) usleep(1000);
        break;
      case 0x0104feef:
      case 0x0204feef:
      case 0x0304feef:
        enable_thread = 0;
        while(active_thread) usleep(1000);
        memset(&addr_ep6, 0, sizeof(addr_ep6));
        addr_ep6.sin_family = AF_INET;
        addr_ep6.sin_addr.s_addr = addr_from.sin_addr.s_addr;
        addr_ep6.sin_port = addr_from.sin_port;
        enable_thread = 1;
        active_thread = 1;
        if(pthread_create(&thread, NULL, handler_ep6, NULL) < 0)
        {
          perror("pthread_create");
          return EXIT_FAILURE;
        }
        pthread_detach(thread);
        break;
    }
  }

  close(sock_ep2);

  return EXIT_SUCCESS;
}

void process_ep2(uint8_t *frame)
{
  uint32_t freq;
  uint8_t pene[3] = {0, 0, 0};
  uint8_t alex[2] = {0, 0};
  int change_pene, change_alex;

  switch(frame[0])
  {
    case 0:
    case 1:
      receivers = ((frame[4] >> 3) & 7) + 1;
      /* set PTT pin */
      if(frame[0] & 1) *gpio_out |= 1;
      else *gpio_out &= ~1;
      /* set preamp pin */
      if(frame[3] & 4) *gpio_out |= 2;
      else *gpio_out &= ~2;

      /* set rx sample rate */
      switch(frame[1] & 3)
      {
        case 0:
          *rx_rate = 1000;
          break;
        case 1:
          *rx_rate = 500;
          break;
        case 2:
          *rx_rate = 250;
          break;
        case 3:
          *rx_rate = 125;
          break;
      }

      /* configure PENELOPE */
      if(i2c_pene)
      {
        change_pene = 0;
        if(pene[0] != (frame[2] >> 1))
        {
          change_pene = 1;
          pene[0] = frame[2] >> 1;
        }
        if(pene[2] != (((frame[4] & 0x03) << 2) | ((frame[3] & 0x60) >> 5)))
        {
          change_pene = 1;
          pene[2] = ((frame[4] & 0x03) << 2) | ((frame[3] & 0x60) >> 5);
        }
        if(pene[1] != (frame[3] & 0x03))
        {
          change_pene = 1;
          pene[1] = frame[3] & 0x03;
        }
        if(change_pene)
        {
          ioctl(i2c_fd, I2C_SLAVE, ADDR_PENE);
          i2c_write(i2c_fd, 0x02, (pene[2] << 9) | (pene[1] << 7) | pene[0]);
        }
      }
      break;
    case 2:
    case 3:
      /* set tx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *tx_freq = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      if(!vna) break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[0] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 6:
    case 7:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[1] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[2] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 10:
    case 11:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[3] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 18:
    case 19:
      /* set VNA mode */
      vna = frame[2] & 128;
      if(vna) *tx_rst |= 2;
      else *tx_rst &= ~2;

      /* configure ALEX */
      if(i2c_alex)
      {
        change_alex = 0;
        if(alex[0] != frame[3])
        {
          change_alex = 1;
          alex[0] = frame[3];
        }
        if(alex[1] != frame[4])
        {
          change_alex = 1;
          alex[1] = frame[4];
        }
        if(change_alex)
        {
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ALEX);
          i2c_write(i2c_fd, 0x02, (alex[1] << 8) | alex[0]);
        }
      }
      break;
  }
}

void *handler_ep6(void *arg)
{
  int i, j, n, m, size;
  int data_offset, header_offset, buffer_offset;
  uint32_t counter;
  uint8_t data0[4096];
  uint8_t data1[4096];
  uint8_t data2[4096];
  uint8_t data3[4096];
  uint8_t buffer[25][1032];
  struct iovec iovec[25][1];
  struct mmsghdr datagram[25];
  uint8_t header[40] =
  {
    127, 127, 127, 0, 0, 33, 17, 21,
    127, 127, 127, 8, 0, 0, 0, 0,
    127, 127, 127, 16, 0, 0, 0, 0,
    127, 127, 127, 24, 0, 0, 0, 0,
    127, 127, 127, 32, 66, 66, 66, 66
  };

  memset(iovec, 0, sizeof(iovec));
  memset(datagram, 0, sizeof(datagram));

  for(i = 0; i < 25; ++i)
  {
    *(uint32_t *)(buffer[i] + 0) = 0x0601feef;
    iovec[i][0].iov_base = buffer[i];
    iovec[i][0].iov_len = 1032;
    datagram[i].msg_hdr.msg_iov = iovec[i];
    datagram[i].msg_hdr.msg_iovlen = 1;
    datagram[i].msg_hdr.msg_name = &addr_ep6;
    datagram[i].msg_hdr.msg_namelen = sizeof(addr_ep6);
  }

  header_offset = 0;
  counter = 0;

  *rx_rst |= 1;
  *rx_rst &= ~1;

  while(1)
  {
    if(!enable_thread) break;

    size = receivers * 6 + 2;
    n = 504 / size;
    m = 256 / n;

    if(*rx_cntr[0] >= 2048)
    {
      *rx_rst |= 1;
      *rx_rst &= ~1;
    }

    while(*rx_cntr[0] < m * n * 4) usleep(1000);

    for(i = 0; i < m * n * 16; i += 8)
    {
       *(uint64_t *)(data0 + i) = *rx_data[0];
       *(uint64_t *)(data1 + i) = *rx_data[1];
       *(uint64_t *)(data2 + i) = *rx_data[2];
       *(uint64_t *)(data3 + i) = *rx_data[3];
    }

    data_offset = 0;
    for(i = 0; i < m; ++i)
    {
      *(uint32_t *)(buffer[i] + 4) = htonl(counter);

      memcpy(buffer[i] + 8, header + header_offset, 8);
      buffer[i][11] |= *gpio_in & 7;
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;
      memset(buffer[i] + 16, 0, 504);

      buffer_offset = 16;
      for(j = 0; j < n; ++j)
      {
        memcpy(buffer[i] + buffer_offset, data0 + data_offset, 6);
        if(size > 8)
        {
          memcpy(buffer[i] + buffer_offset + 6, data1 + data_offset, 6);
        }
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        data_offset += 8;
        buffer_offset += size;
      }

      memcpy(buffer[i] + 520, header + header_offset, 8);
      buffer[i][523] |= *gpio_in & 7;
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;
      memset(buffer[i] + 528, 0, 504);

      buffer_offset = 528;
      for(j = 0; j < n; ++j)
      {
        memcpy(buffer[i] + buffer_offset, data0 + data_offset, 6);
        if(size > 8)
        {
          memcpy(buffer[i] + buffer_offset + 6, data1 + data_offset, 6);
        }
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        data_offset += 8;
        buffer_offset += size;
      }

      ++counter;
    }

    sendmmsg(sock_ep2, datagram, m, 0);
  }

  active_thread = 0;

  return NULL;
}
