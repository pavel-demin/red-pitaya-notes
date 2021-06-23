#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_DAC0 0x60 /* MCP4725 address 0 */
#define ADDR_DAC1 0x61 /* MCP4725 address 1 */

int main(int argc, char *argv[])
{
  int fd;
  char *end;
  long number;
  uint8_t buffer[2];

  errno = 0;
  number = (argc == 2) ? strtol(argv[1], &end, 10) : -1;
  if(errno != 0 || end == argv[1] || number < 0 || number > 4095)
  {
    printf("Usage: i2c-write [0-4095]\n");
    return EXIT_FAILURE;
  }

  if((fd = open("/dev/i2c-0", O_RDWR)) >= 0)
  {
    if(ioctl(fd, I2C_SLAVE_FORCE, ADDR_DAC0) >= 0)
    {
      buffer[0] = number >> 8;
      buffer[1] = number;
      if(write(fd, buffer, 2) > 0) return EXIT_SUCCESS;
    }
  }

  return EXIT_FAILURE;
}
