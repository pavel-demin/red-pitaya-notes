#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CTRL_PORT 1001
#define DATA_PORT 1002
#define MCAST_ADDR "239.0.0.39"

#define LO_MIN   100000
#define LO_MAX 50000000

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int file;
  pid_t pid;
  void *cfg, *sts, *ram;
  char *name = "/dev/mem";

  if((file = open(name, O_RDWR)) < 1)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x40001000);
  ram = mmap(NULL, 2048*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, file, 0x1E000000);

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~15;

  /* set default phase increment */
  *((uint32_t *)(cfg + 4)) = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);

  /* set packet size */
  *((uint32_t *)(cfg + 8)) = 127;

  pid = fork();

  if(pid == 0)
  {
    /* child process */
    int sockCtrl;
    struct sockaddr_in addrCtrl;
    struct ip_mreq mreq;
    uint32_t command = 600000;

    if((sockCtrl = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
    {
      perror("socket");
      return 1;
    }

    /* set up destination address */
    memset(&addrCtrl, 0, sizeof(addrCtrl));
    addrCtrl.sin_family = AF_INET;
    addrCtrl.sin_addr.s_addr = htonl(INADDR_ANY);
    addrCtrl.sin_port=htons(CTRL_PORT);

    /* bind to receive address */
    if(bind(sockCtrl, (struct sockaddr *)&addrCtrl, sizeof(addrCtrl)) < 0)
    {
      perror("bind");
      return 1;
    }

    /* use setsockopt() to request that the kernel join a multicast group */
    mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(sockCtrl, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
      perror("setsockopt");
      return 1;
    }

    while(1)
    {
      if(recvfrom(sockCtrl, (char *)&command, 4, 0, NULL, 0) < 0)
      {
        perror("recvfrom");
        exit(1);
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
  }
  else if(pid > 0)
  {
    /* parent process */
    int sockData;
    struct sockaddr_in addrData;
    int pos, limit, start, status;

    if((sockData = socket(AF_INET, SOCK_DGRAM, 0)) < 1)
    {
      perror("socket");
      return 1;
    }

    /* set up destination address */
    memset(&addrData, 0, sizeof(addrData));
    addrData.sin_family = AF_INET;
    addrData.sin_addr.s_addr = inet_addr(MCAST_ADDR);
    addrData.sin_port = htons(DATA_PORT);

    signal(SIGINT, signal_handler);

    limit = 128;
    while(!interrupted && waitpid(pid, &status, WNOHANG) != pid)
    {
      /* read ram writer position */
      pos = *((uint32_t *)(sts + 0));

      /* send 1024 bytes if ready, otherwise sleep 0.5 ms */
      if((limit > 0 && pos > limit) || (limit == 0 && pos < 384))
      {
        start = limit > 0 ? limit*8 - 1024 : 3072;
        sendto(sockData, ram + start, 1024, 0, (struct sockaddr *)&addrData, sizeof(addrData));
        limit += 128;
        if(limit == 512) limit = 0;
      }
      else
      {
        usleep(500);
      }
    }

    if(waitpid(pid, &status, WNOHANG) != pid) kill(pid, SIGTERM);
    usleep(1000);
    if(waitpid(pid, &status, WNOHANG) != pid) kill(pid, SIGKILL);

    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~15;
  }
  else
  {
    perror("fork");
    return 1;
  }

  munmap(ram, sysconf(_SC_PAGESIZE));
  munmap(sts, sysconf(_SC_PAGESIZE));
  munmap(cfg, sysconf(_SC_PAGESIZE));

  return 0;
}
