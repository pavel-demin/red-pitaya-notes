#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1001

int interrupted = 0;

void signal_handler(int sig)
{
  interrupted = 1;
}

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  void *cfg, *sts, *trg, *hst[2], *ram, *buf;
  char *name = "/dev/mem";
  struct sockaddr_in addr;
  int yes = 1;
  uint32_t start, pre, tot;
  uint64_t command, data;
  uint8_t code, chan;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  trg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  hst[0] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  hst[1] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);
  buf = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

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
  addr.sin_port = htons(TCP_PORT);

  if(bind(sock_server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  listen(sock_server, 1024);

  while(!interrupted)
  {
    /* enter reset mode */
    *(uint16_t *)(cfg + 0) &= ~63;

    signal(SIGINT, SIG_DFL);

    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *(uint16_t *)(cfg + 0) |= 63;

    while(!interrupted)
    {
      if(recv(sock_client, &command, 8, MSG_WAITALL) <= 0) break;
      code = (uint8_t)(command >> 56) & 0xff;
      chan = (uint8_t)(command >> 52) & 0xf;
      data = (uint64_t)(command & 0xfffffffffffffULL);

      if(code == 0)
      {
        /* reset all */
        *(uint16_t *)(cfg + 0) &= ~63;
        *(uint16_t *)(cfg + 0) |= 63;
      }
      else if(code == 1)
      {
        /* reset histogram */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 0) &= ~1;
          *(uint16_t *)(cfg + 0) |= 1;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 0) &= ~2;
          *(uint16_t *)(cfg + 0) |= 2;
        }
      }
      else if(code == 2)
      {
        /* reset timer */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 0) &= ~4;
          *(uint16_t *)(cfg + 0) |= 4;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 0) &= ~8;
          *(uint16_t *)(cfg + 0) |= 8;
        }
      }
      else if(code == 3)
      {
        /* reset scope */
        *(uint16_t *)(cfg + 0) &= ~48;
        *(uint16_t *)(cfg + 0) |= 48;
      }
      else if(code == 4)
      {
        /* set rate */
        *(uint16_t *)(cfg + 2) = data;
      }
      else if(code == 5)
      {
        /* set pha delay */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 4) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 10) = data;
        }
      }
      else if(code == 6)
      {
        /* set pha min threshold */
        if(chan == 0)
        {
          *(int16_t *)(cfg + 6) = data;
        }
        else if(chan == 1)
        {
          *(int16_t *)(cfg + 12) = data;
        }
      }
      else if(code == 7)
      {
        /* set pha max threshold */
        if(chan == 0)
        {
          *(int16_t *)(cfg + 8) = data;
        }
        else if(chan == 1)
        {
          *(int16_t *)(cfg + 14) = data;
        }
      }
      else if(code == 8)
      {
        /* set timer */
        if(chan == 0)
        {
          *(uint64_t *)(cfg + 16) = data;
          *(uint16_t *)(cfg + 0) |= 128;
          *(uint16_t *)(cfg + 0) &= ~128;
        }
        else if(chan == 1)
        {
          *(uint64_t *)(cfg + 24) = data;
          *(uint16_t *)(cfg + 0) |= 512;
          *(uint16_t *)(cfg + 0) &= ~512;
        }
      }
      else if(code == 9)
      {
        /* start timer */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 0) |= 64;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 0) |= 256;
        }
      }
      else if(code == 10)
      {
        /* stop timer */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 0) &= ~64;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 0) &= ~256;
        }
      }
      else if(code == 11)
      {
        /* read timer */
        if(chan == 0)
        {
          memcpy(buf, sts + 0, 8);
          if(send(sock_client, buf, 8, MSG_NOSIGNAL) < 0) break;
        }
        else if(chan == 1)
        {
          memcpy(buf, sts + 8, 8);
          if(send(sock_client, buf, 8, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 12)
      {
        /* read histogram */
        if(chan == 0)
        {
          memcpy(buf, hst[0], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
        else if(chan == 1)
        {
          memcpy(buf, hst[1], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 13)
      {
        /* set trigger channel */
        if(chan == 0)
        {
          *(uint32_t *)(trg + 64) = 0;
          *(uint32_t *)(trg + 0) = 2;
        }
        else if(chan == 1)
        {
          *(uint32_t *)(trg + 64) = 1;
          *(uint32_t *)(trg + 0) = 2;
        }
      }
      else if(code == 14)
      {
        /* set trigger edge (0 for negative, 1 for positive) */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 0) &= ~256;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 0) |= 256;
        }
      }
      else if(code == 15)
      {
        /* set trigger threshold */
        *(int16_t *)(cfg + 40) = data;
      }
      else if(code == 16)
      {
        /* set number of samples before trigger */
        *(uint32_t *)(cfg + 32) = data - 1;
      }
      else if(code == 17)
      {
        /* set total number of samples */
        *(uint16_t *)(cfg + 36) = data - 1;
      }
      else if(code == 18)
      {
        /* read oscilloscope status */
        *(uint32_t *)buf = *(uint32_t *)(sts + 16) & 1;
        if(send(sock_client, buf, 4, MSG_NOSIGNAL) < 0) break;
      }
      else if(code == 19)
      {
        /* read oscilloscope data */
        pre = *(uint32_t *)(cfg + 32) + 1;
        tot = *(uint32_t *)(cfg + 36) + 1;
        start = *(uint32_t *)(sts + 16) >> 1;
        start = (start - pre) & 0x007FFFC0;
        if(start + tot <= 0x007FFFFF)
        {
          memcpy(buf, ram + start * 4, tot * 4);
          if(send(sock_client, buf, tot * 4, MSG_NOSIGNAL) < 0) break;
        }
        else
        {
          memcpy(buf, ram + start * 4, (0x007FFFFF - start) * 4);
          if(send(sock_client, buf, (0x007FFFFF - start) * 4, MSG_NOSIGNAL) < 0) break;
          memcpy(buf, ram, (start + tot - 0x007FFFFF) * 4);
          if(send(sock_client, buf, (start + tot - 0x007FFFFF) * 4, MSG_NOSIGNAL) < 0) break;
        }
      }
    }

    close(sock_client);
  }

  close(sock_server);

  /* enter reset mode */
  *(uint16_t *)(cfg + 0) &= ~63;

  return EXIT_SUCCESS;
}
