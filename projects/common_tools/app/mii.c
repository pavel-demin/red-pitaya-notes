#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>

#include <linux/mii.h>
#include <linux/sockios.h>

int main(int argc, char *argv[])
{
  int fd;
  struct ifreq ifr;
  struct mii_ioctl_data *mii;

  if(argc < 3 || argc > 4)
  {
    fprintf(stderr, "Usage: mii interface register [value]\n");
    return EXIT_FAILURE;
  }

  if((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, argv[1], IFNAMSIZ - 1);

  if(ioctl(fd, SIOCGMIIPHY, &ifr) < 0)
  {
    perror("SIOCGMIIPHY");
    return EXIT_FAILURE;
  }

  mii = (struct mii_ioctl_data *)&ifr.ifr_data;

  mii->reg_num = (uint16_t)strtoul(argv[2], NULL, 16);

  if(argc == 3)
  {
    if(ioctl(fd, SIOCGMIIREG, &ifr) < 0)
    {
      perror("SIOCGMIIREG");
      return EXIT_FAILURE;
    }
    printf("0x%04x\n", mii->val_out);
  }
  else
  {
    mii->val_in = (uint16_t)strtoul(argv[3], NULL, 16);
    if(ioctl(fd, SIOCSMIIREG, &ifr) < 0)
    {
      perror("SIOCSMIIREG");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
