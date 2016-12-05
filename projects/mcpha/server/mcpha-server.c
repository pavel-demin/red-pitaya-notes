#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TCP_PORT 1001

int main(int argc, char *argv[])
{
  int fd, sock_server, sock_client;
  volatile void *sts, *cfg, *gen;
  void *hst[2], *ram, *buf;
  volatile uint8_t *rst[4];
  volatile uint32_t *trg;
  struct sockaddr_in addr;
  int yes = 1;
  uint32_t start, pre, tot;
  uint64_t command, data;
  uint8_t code, chan;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  trg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  hst[0] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  hst[1] = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);
  gen = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40030000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);
  buf = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  rst[0] = cfg + 0;
  rst[1] = cfg + 1;
  rst[2] = cfg + 2;
  rst[3] = cfg + 3;

  /* set sample rate */
  *(uint16_t *)(cfg + 4) = 125;

  /* set trigger channel */
  trg[16] = 0;
  trg[0] = 2;

  /* reset timers and histograms */
  *rst[0] &= ~3;
  *rst[0] |= 3;
  *rst[1] &= ~3;
  *rst[1] |= 3;

  /* reset oscilloscope */
  *rst[2] &= ~3;
  *rst[2] |= 3;

  /* reset generator */
  *rst[3] &= ~128;
  *rst[3] |= 128;

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

  while(1)
  {
    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return EXIT_FAILURE;
    }

    while(1)
    {
      if(recv(sock_client, &command, 8, MSG_WAITALL) <= 0) break;
      code = (uint8_t)(command >> 56) & 0xff;
      chan = (uint8_t)(command >> 52) & 0xf;
      data = (uint64_t)(command & 0xfffffffffffffULL);

      if(code == 0)
      {
        /* reset timer */
        if(chan == 0)
        {
          *rst[0] &= ~2;
          *rst[0] |= 2;
        }
        else if(chan == 1)
        {
          *rst[1] &= ~2;
          *rst[1] |= 2;
        }
      }
      else if(code == 1)
      {
        /* reset histogram*/
        if(chan == 0)
        {
          *rst[0] &= ~1;
          *rst[0] |= 1;
        }
        else if(chan == 1)
        {
          *rst[1] &= ~1;
          *rst[1] |= 1;
        }
      }
      else if(code == 2)
      {
        /* reset oscilloscope */
        *rst[2] &= ~3;
        *rst[2] |= 3;
      }
      else if(code == 3)
      {
        /* reset generator */
        *rst[3] &= ~128;
        *rst[3] |= 128;
      }
      else if(code == 4)
      {
        /* set sample rate */
        *(uint16_t *)(cfg + 4) = data;
      }
      else if(code == 5)
      {
        /* set baseline mode (0 for none, 1 for auto) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst[0] &= ~4;
          }
          else if(data == 1)
          {
            *rst[0] |= 4;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst[1] &= ~4;
          }
          else if(data == 1)
          {
            *rst[1] |= 4;
          }
        }
      }
      else if(code == 6)
      {
        /* set baseline level */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 16) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 32) = data;
        }
      }
      else if(code == 7)
      {
        /* set pha delay */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 18) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 34) = data;
        }
      }
      else if(code == 8)
      {
        /* set pha min threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 20) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 36) = data;
        }
      }
      else if(code == 9)
      {
        /* set pha max threshold */
        if(chan == 0)
        {
          *(uint16_t *)(cfg + 22) = data;
        }
        else if(chan == 1)
        {
          *(uint16_t *)(cfg + 38) = data;
        }
      }
      else if(code == 10)
      {
        /* set timer */
        if(chan == 0)
        {
          *(uint64_t *)(cfg + 8) = data;
        }
        else if(chan == 1)
        {
          *(uint64_t *)(cfg + 24) = data;
        }
      }
      else if(code == 11)
      {
        /* set timer mode (0 for stop, 1 for running) */
        if(chan == 0)
        {
          if(data == 0)
          {
            *rst[0] &= ~8;
          }
          else if(data == 1)
          {
            *rst[0] |= 8;
          }
        }
        else if(chan == 1)
        {
          if(data == 0)
          {
            *rst[1] &= ~8;
          }
          else if(data == 1)
          {
            *rst[1] |= 8;
          }
        }
      }
      else if(code == 12)
      {
        /* read timer */
        if(chan == 0)
        {
          data = *(uint64_t *)(sts + 12);
          if(send(sock_client, &data, 8, MSG_NOSIGNAL) < 0) break;
        }
        else if(chan == 1)
        {
          data = *(uint64_t *)(sts + 20);
          if(send(sock_client, &data, 8, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(code == 13)
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
      else if(code == 14)
      {
        /* set trigger source (0 for channel 1, 1 for channel 2) */
        if(chan == 0)
        {
          trg[16] = 0;
          trg[0] = 2;
        }
        else if(chan == 1)
        {
          trg[16] = 1;
          trg[0] = 2;
        }
      }
      else if(code == 15)
      {
        /* set trigger slope (0 for rising, 1 for falling) */
        if(data == 0)
        {
          *rst[2] &= ~4;
        }
        else if(data == 1)
        {
          *rst[2] |= 4;
        }
      }
      else if(code == 16)
      {
        /* set trigger mode (0 for normal, 1 for auto) */
        if(data == 0)
        {
          *rst[2] &= ~8;
        }
        else if(data == 1)
        {
          *rst[2] |= 8;
        }
      }
      else if(code == 17)
      {
        /* set trigger level */
        *(uint16_t *)(cfg + 80) = data;
      }
      else if(code == 18)
      {
        /* set number of samples before trigger */
        *(uint32_t *)(cfg + 72) = data - 1;
      }
      else if(code == 19)
      {
        /* set total number of samples */
        *(uint32_t *)(cfg + 76) = data - 1;
      }
      else if(code == 20)
      {
        /* start oscilloscope */
        *rst[2] |= 16;
        *rst[2] &= ~16;
      }
      else if(code == 21)
      {
        /* read oscilloscope status */
        *(uint32_t *)buf = *(uint32_t *)(sts + 44) & 1;
        if(send(sock_client, buf, 4, MSG_NOSIGNAL) < 0) break;
      }
      else if(code == 22)
      {
        /* read oscilloscope data */
        pre = *(uint32_t *)(cfg + 72) + 1;
        tot = *(uint32_t *)(cfg + 76) + 1;
        start = *(uint32_t *)(sts + 44) >> 1;
        start = (start - pre) & 0x007FFFFF;
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

  return EXIT_SUCCESS;
}
