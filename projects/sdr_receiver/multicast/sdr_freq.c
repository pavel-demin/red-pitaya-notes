#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define FREQ_PORT 1001
#define MCAST_ADDR "239.0.0.39"

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  struct sockaddr_in addr;
  int addrlen = sizeof(addr);
  int fd;
  struct ip_mreq mreq;
  long freq = 600000;
  void *cfg, *sts, *ram;
  char *name = "/dev/mem";

  if((fd = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);

  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
  {
    perror("socket");
    return 1;
  }

  /* set up destination address */
  memset(&addr,0,sizeof(addr));
  addr.sin_family=AF_INET;
  addr.sin_addr.s_addr=htonl(INADDR_ANY);
  addr.sin_port=htons(FREQ_PORT);

  /* bind to receive address */
  if(bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0)
  {
    perror("bind");
    return 1;
  }
     
  /* use setsockopt() to request that the kernel join a multicast group */
  mreq.imr_multiaddr.s_addr=inet_addr(MCAST_ADDR);
  mreq.imr_interface.s_addr=htonl(INADDR_ANY);
  if(setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
  {
    perror("setsockopt");
    return 1;
  }

  while(!interrupted)
  {
    if(recvfrom(fd, (char *) &freq, 4, 0, NULL, 0) < 0)
    {
      perror("recvfrom");
      return 1;
    }
    *((uint32_t *)(cfg + 4)) = roundf(freq/125.0e6*(1<<22)-1);
  }

  munmap(cfg, sysconf(_SC_PAGESIZE));

  return 0;
}
