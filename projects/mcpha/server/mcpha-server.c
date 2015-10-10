#include <stdio.h>
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
  pid_t pid;
  void *cfg, *sts, *trg, *hst[2], *ram, *buf;
  char *name = "/dev/mem";
  int size = 0;
  struct sockaddr_in addr;
  int yes = 1;
  uint32_t start, pre, tot;
  char command[12];
  uint8_t *code = (uint8_t *)(command + 0);
  uint8_t *chan = (uint8_t *)(command + 1);
  uint16_t *data16 = (uint16_t *)(command + 2);
  uint32_t *data32 = (uint32_t *)(command + 2);
  uint64_t *data64 = (uint64_t *)(command + 2);

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return 1;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  trg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  hst[0] = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  hst[1] = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);
  ram = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x1E000000);
  buf = mmap(NULL, 8192*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

  if((sock_server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket");
    return 1;
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
    return 1;
  }

  listen(sock_server, 1024);

  while(!interrupted)
  {
    /* enter reset mode */
    *((uint32_t *)(cfg + 0)) &= ~7;

    signal(SIGINT, SIG_DFL);

    if((sock_client = accept(sock_server, NULL, NULL)) < 0)
    {
      perror("accept");
      return 1;
    }

    signal(SIGINT, signal_handler);

    /* enter normal operating mode */
    *((uint32_t *)(cfg + 0)) |= 7;

    while(!interrupted)
    {
      if(recv(sock_client, command, 12, MSG_WAITALL) <= 0) break;
      if(*code == 0)
      {
        /* reset */
        *((uint32_t *)(cfg + 0)) &= ~7;
        *((uint32_t *)(cfg + 0)) |= 7;
      }
      else if(*code == 1)
      {
        /* set pha delay */
        if(*chan == 0)
        {
          *((uint16_t *)(cfg + 6)) = *data16;
        }
        else if(*chan == 1)
        {
          *((uint16_t *)(cfg + 12)) = *data16;
        }
      }
      else if(*code == 2)
      {
        /* set pha min threshold */
        if(*chan == 0)
        {
          *((int16_t *)(cfg + 8)) = *data16;
        }
        else if(*chan == 1)
        {
          *((int16_t *)(cfg + 14)) = *data16;
        }
      }
      else if(*code == 3)
      {
        /* set pha max threshold */
        if(*chan == 0)
        {
          *((int16_t *)(cfg + 10)) = *data16;
        }
        else if(*chan == 1)
        {
          *((int16_t *)(cfg + 16)) = *data16;
        }
      }
      else if(*code == 4)
      {
        /* set timer */
        if(*chan == 0)
        {
          *((uint64_t *)(cfg + 20)) = *data64;
          *((uint16_t *)(cfg + 0)) |= 16;
          *((uint16_t *)(cfg + 0)) &= ~16;
        }
        else if(*chan == 1)
        {
          *((uint64_t *)(cfg + 28)) = *data64;
          *((uint16_t *)(cfg + 0)) |= 64;
          *((uint16_t *)(cfg + 0)) &= ~64;
        }
      }
      else if(*code == 5)
      {
        /* start timer */
        if(*chan == 0)
        {
          *((uint16_t *)(cfg + 0)) |= 8;
        }
        else if(*chan == 1)
        {
          *((uint16_t *)(cfg + 0)) |= 32;
        }
      }
      else if(*code == 6)
      {
        /* stop timer */
        if(*chan == 0)
        {
          *((uint16_t *)(cfg + 0)) &= ~8;
        }
        else if(*chan == 1)
        {
          *((uint16_t *)(cfg + 0)) &= ~32;
        }
      }
      else if(*code == 7)
      {
        /* reset histogram */
        if(*chan == 0)
        {
          *((uint16_t *)(cfg + 0)) &= ~1;
          *((uint16_t *)(cfg + 0)) |= 1;
        }
        else if(*chan == 1)
        {
          *((uint16_t *)(cfg + 0)) &= ~2;
          *((uint16_t *)(cfg + 0)) |= 2;
        }
      }
      else if(*code == 8)
      {
        /* read timer */
        if(*chan == 0)
        {
          memcpy(buf, sts + 0, 8);
          if(send(sock_client, buf, 8, MSG_NOSIGNAL) < 0) break;
        }
        else if(*chan == 1)
        {
          memcpy(buf, sts + 8, 8);
          if(send(sock_client, buf, 8, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(*code == 9)
      {
        /* read histogram */
        if(*chan == 0)
        {
          memcpy(buf, hst[0], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
        else if(*chan == 1)
        {
          memcpy(buf, hst[1], 65536);
          if(send(sock_client, buf, 65536, MSG_NOSIGNAL) < 0) break;
        }
      }
      else if(*code == 10)
      {
        /* set trigger channel */
        if(*chan == 0)
        {
          *((uint32_t *)(trg + 64)) = 0;
          *((uint32_t *)(trg + 0)) = 2;
        }
        else if(*chan == 1)
        {
          *((uint32_t *)(trg + 64)) = 1;
          *((uint32_t *)(trg + 0)) = 2;
        }
      }
      else if(*code == 11)
      {
        /* set trigger edge (0 for negative, 1 for positive) */
        if(*chan == 0)
        {
          *((uint16_t *)(cfg + 0)) &= ~128;
        }
        else if(*chan == 1)
        {
          *((uint16_t *)(cfg + 0)) |= 128;
        }
      }
      else if(*code == 12)
      {
        /* set trigger threshold */
        *((int16_t *)(cfg + 18)) = *data16;
      }
      else if(*code == 13)
      {
        /* set number of samples before trigger */
        *((uint32_t *)(cfg + 36)) = *data32;
      }
      else if(*code == 14)
      {
        /* set total number of samples */
        *((uint16_t *)(cfg + 40)) = *data32;
      }
      else if(*code == 15)
      {
        /* read oscilloscope status */
        *(uint32_t *)buf = *((uint32_t *)(sts + 16)) & 1;
        if(send(sock_client, buf, 4, MSG_NOSIGNAL) < 0) break;
      }
      else if(*code == 16)
      {
        /* read oscilloscope data */
        pre = *((uint32_t *)(cfg + 36));
        tot = *((uint32_t *)(cfg + 40));;
        start = *((uint32_t *)(sts + 0)) >> 1;
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

  /* enter reset mode */
  *((uint32_t *)(cfg + 0)) &= ~15;

  return 0;
}
