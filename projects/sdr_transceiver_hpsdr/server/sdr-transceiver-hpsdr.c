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

uint32_t *rx_freq[2], *rx_rate[2], *tx_freq;
uint16_t *rx_cntr[2], *tx_cntr;
uint8_t *gpio, *rx_rst, *tx_rst;
void *rx_data[2], *tx_data;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int receivers = 1;

int sock_ep2;
struct sockaddr_in addr_ep6;

int enable_thread = 0;
int active_thread = 0;

int vna = 0;

void process_ep2(char *frame);
void *handler_ep6(void *arg);

int main(int argc, char *argv[])
{
  int fd, i;
  ssize_t size;
  pthread_t thread;
  void *cfg, *sts;
  char *name = "/dev/mem";
  char buffer[1032];
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

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data[0] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rx_data[1] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
  tx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);

  *(uint32_t *)(tx_data + 8) = 165;

  rx_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  gpio = ((uint8_t *)(cfg + 2));

  rx_freq[0] = ((uint32_t *)(cfg + 4));
  rx_rate[0] = ((uint32_t *)(cfg + 8));
  rx_cntr[0] = ((uint16_t *)(sts + 0));

  rx_freq[1] = ((uint32_t *)(cfg + 12));
  rx_rate[1] = ((uint32_t *)(cfg + 16));
  rx_cntr[1] = ((uint16_t *)(sts + 2));

  tx_freq = ((uint32_t *)(cfg + 28));
  tx_cntr = ((uint16_t *)(sts + 4));

  /* set I/Q data for the VNA mode */
  *((uint64_t *)(cfg + 20)) = 2000000;
  *tx_rst &= ~2;

  /* set PTT pin to low */
  *gpio = 0;

  /* set default rx phase increment */
  *rx_freq[0] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[1] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate[0] = 1000;
  *rx_rate[1] = 1000;

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
        if(*gpio)
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

void process_ep2(char *frame)
{
  uint32_t freq;

  switch(frame[0])
  {
    case 0:
    case 1:
      receivers = ((frame[4] >> 3) & 7) + 1;
      /* set PTT pin */
      *gpio = frame[0] & 1;
      /* set rx sample rate */
      switch(frame[1] & 3)
      {
        case 0:
          *rx_rate[0] = 1000;
          *rx_rate[1] = 1000;
          break;
        case 1:
          *rx_rate[0] = 500;
          *rx_rate[1] = 500;
          break;
        case 2:
          *rx_rate[0] = 250;
          *rx_rate[1] = 250;
          break;
        case 3:
          *rx_rate[0] = 125;
          *rx_rate[1] = 125;
          break;
      }
      break;
    case 2:
    case 3:
      /* set tx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *tx_freq = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      if(!vna) break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[0] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 6:
    case 7:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[1] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 18:
    case 19:
      /* set VNA mode */
      vna = frame[2] & 128;
      if(vna) *tx_rst |= 2;
      else *tx_rst &= ~2;
      break;
  }
}

void *handler_ep6(void *arg)
{
  int i, j, n, m, size;
  int data_offset, header_offset, buffer_offset;
  uint32_t counter;
  char data0[4096];
  char data1[4096];
  char buffer[25][1032];
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

    memcpy(data0, rx_data[0], m * n * 16);
    memcpy(data1, rx_data[1], m * n * 16);

    data_offset = 0;
    for(i = 0; i < m; ++i)
    {
      *(uint32_t *)(buffer[i] + 4) = htonl(counter);

      memcpy(buffer[i] + 8, header + header_offset, 8);
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
        data_offset += 8;
        buffer_offset += size;
      }

      memcpy(buffer[i] + 520, header + header_offset, 8);
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
