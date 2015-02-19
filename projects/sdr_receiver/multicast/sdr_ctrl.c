#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FREQ_PORT 1001
#define MCAST_ADDR "239.0.0.39"

#define LO_MIN   100000
#define LO_MAX 50000000

void *cfg;

void signal_handler(int sig)
{
  *((uint32_t *)(cfg + 0)) &= ~15;
  exit(0);
}

int main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int file, sock;
  struct ip_mreq mreq;
  uint32_t command = 600000;
  char *name = "/dev/mem";

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
  {
    perror("socket");
    return 1;
  }

  /* set up destination address */
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port=htons(FREQ_PORT);

  /* bind to receive address */
  if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return 1;
  }

  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
  {
    perror("setsockopt");
    return 1;
  }

  if((file = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40000000);

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~15;

  /* set default phase increment */
  *((uint32_t *)(cfg + 4)) = (uint32_t)floor(command/125.0e6*(1<<30)+0.5);

  /* set packet size */
  *((uint32_t *)(cfg + 8)) = 127;

  signal(SIGINT, signal_handler);

  while(1)
  {
    if(recvfrom(sock, (char *)&command, 4, 0, NULL, 0) < 0)
    {
      perror("recvfrom");
      break;
    }

    switch(command>>30)
    {
      case 0:
        /* set phase increment */
        if(command < LO_MIN || command > LO_MAX) continue;
        *((uint32_t *)(cfg + 4)) = (uint32_t)floor(command/125.0e6*(1<<30)+0.5);
        break;
      case 1:
        /* enter normal operating mode */
        *((uint32_t *)(cfg + 0)) |= 15;
        break;
      case 2:
        /* enter reset mode */
        *((uint32_t *)(cfg + 0)) &= ~15;
        break;
    }
  }

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~15;

  return 0;
}
