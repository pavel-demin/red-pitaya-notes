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

volatile uint32_t *rx_freq;
volatile uint16_t *rx_rate, *rx_cntr;
volatile uint8_t *rx_rst;
volatile uint8_t *rx_data;

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
  volatile void *cfg, *sts;
  volatile uint8_t *rx_sel;
  char *end;
  uint8_t buffer[1032];
  uint8_t reply[20] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 25, 1, 'R', '_', 'P', 'I', 'T', 'A', 'Y', 'A', 8};
  uint32_t code;
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from;
  socklen_t size_from;
  int yes = 1;
  uint8_t chan = 0;
  long number;

  for(i = 0; i < 8; ++i)
  {
    errno = 0;
    number = (argc == 9) ? strtol(argv[i + 1], &end, 10) : -1;
    if(errno != 0 || end == argv[i + 1] || number < 1 || number > 2)
    {
      fprintf(stderr, "Usage: sdr-receiver-hpsdr 1|2 1|2 1|2 1|2 1|2 1|2 1|2 1|2\n");
      return EXIT_FAILURE;
    }
    chan |= (number - 1) << i;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  rx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);

  rx_rst = (uint8_t *)(cfg + 0);
  rx_sel = (uint8_t *)(cfg + 1);
  rx_rate = (uint16_t *)(cfg + 2);
  rx_freq = (uint32_t *)(cfg + 4);

  rx_cntr = (uint16_t *)(sts + 0);

  /* set default rx phase increment */
  for(i = 0; i < 8; ++i)
  {
    rx_freq[i] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
  }

  /* set default rx sample rate */
  *rx_rate = 1000;

  *rx_sel = chan;

  if((sock_ep2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  memset(&hwaddr, 0, sizeof(hwaddr));
  strncpy(hwaddr.ifr_name, "eth0", IFNAMSIZ - 1);
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

    memcpy(&code, buffer, 4);
    switch(code)
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
      }
      break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[0] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 6:
    case 7:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[1] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[2] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 10:
    case 11:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[3] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 12:
    case 13:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[4] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 14:
    case 15:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[5] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 16:
    case 17:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[6] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 36:
    case 37:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[7] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
  }
}

void *handler_ep6(void *arg)
{
  int i, j, n, m, size;
  int data_offset, header_offset;
  uint32_t counter;
  uint8_t data[6 * 2048];
  uint8_t buffer[12 * 1032];
  uint8_t *pointer;
  struct iovec iovec[12][1];
  struct mmsghdr datagram[12];
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

  for(i = 0; i < 12; ++i)
  {
    *(uint32_t *)(buffer + i * 1032 + 0) = 0x0601feef;
    iovec[i][0].iov_base = buffer + i * 1032;
    iovec[i][0].iov_len = 1032;
    datagram[i].msg_hdr.msg_iov = iovec[i];
    datagram[i].msg_hdr.msg_iovlen = 1;
    datagram[i].msg_hdr.msg_name = &addr_ep6;
    datagram[i].msg_hdr.msg_namelen = sizeof(addr_ep6);
  }

  header_offset = 0;
  counter = 0;

  *rx_rst &= ~1;
  *rx_rst |= 1;

  while(1)
  {
    if(!enable_thread) break;

    size = receivers * 6 + 2;
    n = 504 / size;
    m = 128 / n;

    if(*rx_cntr >= 1024)
    {
      *rx_rst &= ~1;
      *rx_rst |= 1;
    }

    while(*rx_cntr < m * n * 2) usleep(1000);

    memcpy(data, rx_data, m * n * 96);

    data_offset = 0;
    for(i = 0; i < m; ++i)
    {
      *(uint32_t *)(buffer + i * 1032 + 4) = htonl(counter);
      ++counter;
    }

    for(i = 0; i < m * 2; ++i)
    {
      pointer = buffer + i * 516 - i % 2 * 4 + 8;
      memcpy(pointer, header + header_offset, 8);
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;

      pointer += 8;
      memset(pointer, 0, 504);
      for(j = 0; j < n; ++j)
      {
        memcpy(pointer, data + data_offset, size - 2);
        data_offset += 48;
        pointer += size;
      }
    }

    sendmmsg(sock_ep2, datagram, m, 0);
  }

  active_thread = 0;

  return NULL;
}
