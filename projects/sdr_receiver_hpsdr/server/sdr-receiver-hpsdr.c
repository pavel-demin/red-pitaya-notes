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

uint32_t *rx_freq[6], *rx_rate;
uint16_t *rx_cntr[6];
uint8_t *rx_rst;
uint64_t *rx_data[6];

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int receivers = 1;

int sock_ep2;
struct sockaddr_in addr_ep6;

int enable_thread = 0;
int active_thread = 0;

void process_ep2(uint8_t *frame);
void *handler_ep6(void *arg);

int main(int argc, char *argv[])
{
  int fd, i;
  ssize_t size;
  pthread_t thread;
  void *cfg, *sts, *mux, *ptr;
  char *end, *name = "/dev/mem";
  uint8_t buffer[1032];
  uint8_t reply[20] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 25, 1, 'R', 'T', 'L', '_', 'N', '1', 'G', 'P', 6};
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from;
  socklen_t size_from;
  int yes = 1;
  int val, chan[6] = {1, 1, 1, 1, 1, 1};
  long number;

  for(i = 0; i < 6; ++i)
  {
    errno = 0;
    number = (argc == 7) ? strtol(argv[i + 1], &end, 10) : -1;
    if(errno != 0 || end == argv[i + 1] || number < 1 || number > 2)
    {
      printf("Usage: sdr-transceiver-hpsdr 1|2 1|2 1|2 1|2 1|2 1|2\n");
      return EXIT_FAILURE;
    }
    chan[i] = number;
  }

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);

  for(i = 0; i < 6; ++i)
  {
    rx_data[i] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000 + i * 0x2000);
    rx_freq[i] = ((uint32_t *)(cfg + 8 + i * 4));
    rx_cntr[i] = ((uint16_t *)(sts + 12 + i * 2));

    /* set default rx phase increment */
    *rx_freq[i] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
  }

  for(i = 0; i < 6; ++i)
  {
    ptr = mux + 64 + i * 4;
    val = i * 2 + chan[i] - 1;
    *(uint32_t *)ptr = val;
  }

  *(uint32_t *)mux = 2;

  rx_rst = ((uint8_t *)(cfg + 0));

  rx_rate = ((uint32_t *)(cfg + 4));

  /* set default rx sample rate */
  *rx_rate = 1000;

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
        process_ep2(buffer + 11);
        process_ep2(buffer + 523);
        break;
      case 0x0002feef:
        reply[2] = 2 + active_thread;
        memset(buffer, 0, 60);
        memcpy(buffer, reply, 20);
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

  switch(frame[0])
  {
    case 0:
    case 1:
      receivers = ((frame[4] >> 3) & 7) + 1;

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
      break;
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
    case 12:
    case 13:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[4] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 14:
    case 15:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[5] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
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
  uint8_t data4[4096];
  uint8_t data5[4096];
  uint8_t buffer[25][1032];
  struct iovec iovec[25][1];
  struct mmsghdr datagram[25];
  uint8_t header[40] =
  {
    127, 127, 127, 0, 0, 33, 17, 25,
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
      *(uint64_t *)(data4 + i) = *rx_data[4];
      *(uint64_t *)(data5 + i) = *rx_data[5];
    }

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
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        if(size > 26)
        {
          memcpy(buffer[i] + buffer_offset + 24, data4 + data_offset, 6);
        }
        if(size > 32)
        {
          memcpy(buffer[i] + buffer_offset + 30, data5 + data_offset, 6);
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
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        if(size > 26)
        {
          memcpy(buffer[i] + buffer_offset + 24, data4 + data_offset, 6);
        }
        if(size > 32)
        {
          memcpy(buffer[i] + buffer_offset + 30, data5 + data_offset, 6);
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
