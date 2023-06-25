/*
19.04.2016 DC2PD: add code for bandpass and antenna switching via I2C.
22.08.2016 DL4AOI: add code for TX level switching via I2C.
22.08.2016 DL4AOI: output first four open collector outputs to the pins DIO4_P - DIO7_P of the extension connector E1.
02.09.2016 ON3VNA: add code for TX level switching via DS1803-10 (I2C).
21.09.2016 DC2PD: add code for controlling AD8331 VGA with MCP4725 DAC (I2C).
02.10.2016 DL9LJ: add code for controlling ICOM IC-735 (UART).
03.12.2016 KA6S: add CW keyer code.
16.08.2017 G8NJJ: add code for controlling G8NJJ Arduino sketch (I2C).
*/

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
#include <termios.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_PENE 0x20 /* PCA9555 address 0 */
#define ADDR_ALEX 0x21 /* PCA9555 address 1 */
#define ADDR_LEVEL 0x22 /* PCA9555 address 2 */
#define ADDR_MISC 0x23 /* PCA9555 address 3 */
#define ADDR_DRIVE 0x28 /* DS1803 address 0 */
#define ADDR_CODEC 0x1A /* WM8731 or TLV320AIC23B address 0 */
#define ADDR_DAC0 0x60 /* MCP4725 address 0 */
#define ADDR_DAC1 0x61 /* MCP4725 address 1 */
#define ADDR_ARDUINO 0x40 /* G8NJJ Arduino sketch */
#define ADDR_NUCLEO 0x55 /* NUCLEO-G071RB */

volatile uint32_t *rx_freq, *tx_freq, *alex, *dac_freq;
volatile uint16_t *rx_rate, *rx_cntr, *tx_cntr, *dac_cntr, *adc_cntr;
volatile int16_t *tx_level, *dac_level;
volatile uint8_t *gpio_in, *gpio_out, *rx_rst, *tx_rst, *lo_rst;
volatile uint32_t *fifo, *codec, *xadc;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int receivers = 1;
int rate = 0;

int sock_ep2;
struct sockaddr_in addr_ep6;

int enable_thread = 0;
int active_thread = 0;

void process_ep2(uint8_t *frame);
void *handler_ep6(void *arg);
void *handler_keyer(void *arg);

/* variables to handle I2C devices */
int i2c_fd;
int i2c_pene = 0;
int i2c_alex = 0;
int i2c_level = 0;
int i2c_misc = 0;
int i2c_drive = 0;
int i2c_codec = 0;
int i2c_dac0 = 0;
int i2c_dac1 = 0;
int i2c_arduino = 0;
int i2c_nucleo = 0;

uint16_t i2c_pene_data = 0;
uint16_t i2c_alex_data = 0;
uint16_t i2c_level_data = 0;
uint16_t i2c_misc_data = 0;
uint16_t i2c_drive_data = 0;
uint16_t i2c_dac0_data = 0xfff;
uint16_t i2c_dac1_data = 0xfff;
uint32_t i2c_nucleo_data[2] = {0, 0};

uint16_t i2c_ard_frx1_data = 0; /* rx 1 freq in kHz */
uint16_t i2c_ard_frx2_data = 0; /* rx 2 freq in kHz */
uint16_t i2c_ard_ftx_data = 0; /* tx freq in kHz */
uint32_t i2c_ard_ocant_data = 0; /* oc output and ant */
uint32_t i2c_ard_txatt_data = 0; /* tx attenuation and oddments  */
uint16_t i2c_ard_rxatt_data = 0; /* rx attenuation */

uint8_t log_table_lookup[256]; /* lookup table from linear scale to
                                  6 bit / 0.5 dB attenuation */

uint8_t i2c_boost_data = 0;

uint8_t dac_level_data = 0;

uint8_t cw_int_data = 0;
uint8_t rx_att_data = 0;
uint8_t tx_mux_data = 0;
uint8_t tx_eer_data = 0;

uint16_t cw_hang = 0;
uint8_t cw_reversed = 0;
uint8_t cw_speed = 25;
uint8_t cw_mode = 0;
uint8_t cw_weight = 50;
uint8_t cw_spacing = 0;
uint8_t cw_delay = 0;
uint8_t cw_ptt = 0;

int cw_memory[2] = {0, 0};
int cw_ptt_delay = 0;

uint16_t rx_sync_data = 0;

ssize_t i2c_write_addr_data8(int fd, uint8_t addr, uint8_t data)
{
  uint8_t buffer[2];
  buffer[0] = addr;
  buffer[1] = data;
  return write(fd, buffer, 2);
}

ssize_t i2c_write_addr_data16(int fd, uint8_t addr, uint16_t data)
{
  uint8_t buffer[3];
  buffer[0] = addr;
  buffer[1] = data;
  buffer[2] = data >> 8;
  return write(fd, buffer, 3);
}

ssize_t i2c_write_addr_data24(int fd, uint8_t addr, uint32_t data)
{
  uint8_t buffer[4];
  buffer[0] = addr;
  buffer[1] = data;
  buffer[2] = data >> 8;
  buffer[3] = data >> 16;
  return write(fd, buffer, 4);
}

ssize_t i2c_write_data16(int fd, uint16_t data)
{
  uint8_t buffer[2];
  buffer[0] = data >> 8;
  buffer[1] = data;
  return write(fd, buffer, 2);
}

ssize_t i2c_write_data40(int fd, uint32_t low, uint32_t high)
{
  uint32_t buffer[2];
  buffer[0] = low;
  buffer[1] = high;
  return write(fd, buffer, 5);
}

uint16_t alex_data_rx = 0;
uint16_t alex_data_tx = 0;
uint16_t alex_data_0 = 0;
uint16_t alex_data_1 = 0;
uint16_t alex_update = 0;

uint32_t freq_data[3] = {0, 0, 0};

/* calculate lookup table from drive scale value to 0.5 dB attenuation units */
void calc_log_lookup()
{
  int index;
  float value;
  uint8_t att;

  log_table_lookup[0] = 63; /* max att if no drive */
  for(index = 1; index < 256; ++index)
  {
    value = -40.0 * log10((float)index / 255.0);
    if(value > 63.0)
    {
      att = 63;
    }
    else
    {
      att = (uint8_t)value;
    }
    log_table_lookup[index] = att;
  }
}

void alex_write()
{
  uint32_t max = freq_data[1] > freq_data[2] ? freq_data[1] : freq_data[2];
  uint16_t manual = (alex_data_1 >> 15) & 0x01;
  uint16_t preamp = manual ? (alex_data_1 >> 6) & 0x01 : max > 50000000;
  uint16_t ptt = alex_data_0 & 0x01;
  uint32_t freq = 0;
  uint16_t hpf = 0, lpf = 0, data = 0;

  if(i2c_codec) return;

  freq = freq_data[1] < freq_data[2] ? freq_data[1] : freq_data[2];

  if(preamp) hpf = 0;
  else if(manual) hpf = alex_data_1 & 0x3f;
  else if(freq < 1416000) hpf = 0x20; /* bypass */
  else if(freq < 6500000) hpf = 0x10; /* 1.5 MHz HPF */
  else if(freq < 9500000) hpf = 0x08; /* 6.5 MHz HPF */
  else if(freq < 13000000) hpf = 0x04; /* 9.5 MHz HPF */
  else if(freq < 20000000) hpf = 0x01; /* 13 MHz HPF */
  else hpf = 0x02; /* 20 MHz HPF */

  data =
    ptt << 15 |
    ((alex_data_0 >> 1) & 0x01) << 14 |
    ((alex_data_0 >> 2) & 0x01) << 13 |
    ((hpf >> 5) & 0x01) << 12 |
    ((alex_data_0 >> 7) & 0x01) << 11 |
    (((alex_data_0 >> 5) & 0x03) == 0x01) << 10 |
    (((alex_data_0 >> 5) & 0x03) == 0x02) << 9 |
    (((alex_data_0 >> 5) & 0x03) == 0x03) << 8 |
    ((hpf >> 2) & 0x07) << 4 |
    preamp << 3 |
    (hpf & 0x03) << 1 |
    1;

  if(alex_data_rx != data)
  {
    alex_data_rx = data;
    *alex = 1 << 16 | data;
  }

  freq = ptt ? freq_data[0] : max;

  if(manual) lpf = (alex_data_1 >> 8) & 0x7f;
  else if(freq > 32000000) lpf = 0x10; /* bypass */
  else if(freq > 22000000) lpf = 0x20; /* 12/10 meters */
  else if(freq > 15000000) lpf = 0x40; /* 17/15 meters */
  else if(freq > 8000000) lpf = 0x01; /* 30/20 meters */
  else if(freq > 4500000) lpf = 0x02; /* 60/40 meters */
  else if(freq > 2400000) lpf = 0x04; /* 80 meters */
  else lpf = 0x08; /* 160 meters */

  data =
    ((lpf >> 4) & 0x07) << 13 |
    ptt << 12 |
    (~(alex_data_1 >> 7) & ptt) << 11 |
    (((alex_data_0 >> 8) & 0x03) == 0x02) << 10 |
    (((alex_data_0 >> 8) & 0x03) == 0x01) << 9 |
    (((alex_data_0 >> 8) & 0x03) == 0x00) << 8 |
    (lpf & 0x0f) << 4 |
    1 << 3;

  if(alex_data_tx != data)
  {
    alex_data_tx = data;
    *alex = 1 << 17 | data;
  }
}

static inline int lower_bound(int *array, int size, int value)
{
  int i = 0, j = size, k;
  while(i < j)
  {
    k = i + (j - i) / 2;
    if(value > array[k]) i = k + 1;
    else j = k;
  }
  return i;
}

uint16_t misc_data_0 = 0;
uint16_t misc_data_1 = 0;
uint16_t misc_data_2 = 0;
uint16_t misc_update = 0;

void misc_write()
{
  uint16_t code[3], data = 0;
  int i, freqs[20] =
  {
     1700000,  2100000,
     3400000,  3900000,
     6900000,  7350000,
     9900000, 10250000,
    13900000, 14450000,
    17950000, 18250000,
    20900000, 21550000,
    24800000, 25100000,
    26900000, 30000000,
    49000000, 55000000
  };

  for(i = 0; i < 3; ++i)
  {
    code[i] = lower_bound(freqs, 20, freq_data[i]);
    code[i] = code[i] % 2 ? code[i] / 2 + 1 : 0;
  }

  data |= (code[0] != code[1]) << 8 | code[2] << 4 | code[1];
  data |= (misc_data_1 & 0x18) << 8 | (misc_data_0 & 0x18) << 6;
  data |= (misc_data_2 & 0x03) << 13;

  if(i2c_misc_data != data)
  {
    i2c_misc_data = data;
    ioctl(i2c_fd, I2C_SLAVE, ADDR_MISC);
    i2c_write_addr_data16(i2c_fd, 0x02, data);
  }
}

uint32_t nucleo_data_0 = 0;
uint32_t nucleo_data_1 = 0;
uint32_t nucleo_data_2 = 0;
uint32_t nucleo_data_3 = 0;
uint32_t nucleo_update = 0;

void nucleo_write()
{
  uint32_t data[2] = {0};
  uint32_t code[3];
  int i, freqs[22] =
  {
     1700000,  2100000,
     3400000,  3900000,
     5250000,  5450000,
     6900000,  7350000,
     9900000, 10250000,
    13900000, 14450000,
    17950000, 18250000,
    20900000, 21550000,
    24800000, 25100000,
    26900000, 30000000,
    49000000, 55000000
  };

  for(i = 0; i < 3; ++i)
  {
    code[i] = lower_bound(freqs, 22, freq_data[i]);
    code[i] = code[i] % 2 ? code[i] / 2 + 1 : 0;
  }

  data[0] = nucleo_data_3 << 25 | nucleo_data_2 << 20 | nucleo_data_1 << 17 | nucleo_data_0 << 1 | (code[0] != code[1]);
  data[1] = code[2] << 4 | code[1];

  if(i2c_nucleo_data[0] != data[0] || i2c_nucleo_data[1] != data[1])
  {
    i2c_nucleo_data[0] = data[0];
    i2c_nucleo_data[1] = data[1];
    ioctl(i2c_fd, I2C_SLAVE, ADDR_NUCLEO);
    i2c_write_data40(i2c_fd, data[0], data[1]);
  }
}

int uart_fd;
uint8_t icom_band_data = 0;

void icom_write()
{
  uint32_t freq = freq_data[0];
  uint8_t band;
  uint8_t buffer[10] = {0xfe, 0xfe, 0x04, 0xe0, 0x05, 0x00, 0x00, 0x08, 0x01, 0xfd};

  if(freq < 2000000) band = 0x01;       /* 160m */
  else if(freq < 4000000) band = 0x03;  /*  80m */
  else if(freq < 8000000) band = 0x07;  /*  40m */
  else if(freq < 11000000) band = 0x10; /*  30m */
  else if(freq < 15000000) band = 0x14; /*  20m */
  else if(freq < 20000000) band = 0x18; /*  17m */
  else if(freq < 22000000) band = 0x21; /*  15m */
  else if(freq < 26000000) band = 0x24; /*  12m */
  else band = 0x28;                     /*  10m */

  switch(band)
  {
    case 0x01:
      buffer[7] = 0x80;
      break;
    case 0x03:
      buffer[7] = 0x50;
      break;
    default:
      buffer[7] = 0x00;
  }

  buffer[8] = band;

  if(icom_band_data != band)
  {
    icom_band_data = band;
    if(uart_fd >= 0) write(uart_fd, buffer, 10);
  }
}

int main(int argc, char *argv[])
{
  int fd, i, j, size;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_t thread;
  volatile void *cfg, *sts;
  volatile int32_t *tx_ramp;
  volatile uint16_t *tx_size, *dac_size;
  volatile int16_t *ps_level, *dac_ramp;
  volatile uint8_t *rx_sel, *tx_sel;
  float scale, ramp[1024], a[4] = {0.35875, 0.48829, 0.14128, 0.01168};
  uint8_t reply[11] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 32, 1};
  uint8_t id[4] = {0xef, 0xfe, 1, 6};
  uint32_t code;
  uint16_t data;
  struct termios tty;
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from[10];
  uint8_t buffer[8][1032];
  struct iovec iovec[8][1];
  struct mmsghdr datagram[8];
  struct timeval tv;
  struct timespec ts;
  int yes = 1;
  char *end;
  uint8_t chan = 0;
  long number;

  for(i = 0; i < 6; ++i)
  {
    errno = 0;
    number = (argc == 7) ? strtol(argv[i + 1], &end, 10) : -1;
    if(errno != 0 || end == argv[i + 1] || number < 1 || number > 2)
    {
      fprintf(stderr, "Usage: sdr-transceiver-hpsdr 1|2 1|2 1|2 1|2 1|2 1|2\n");
      return EXIT_FAILURE;
    }
    chan |= (number - 1) << i;
  }

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  if((uart_fd = open("/dev/ttyPS1", O_RDWR | O_NOCTTY | O_NDELAY)) >= 0)
  {
    tcgetattr(uart_fd, &tty);
    cfsetspeed(&tty, (speed_t)B1200);
    cfmakeraw(&tty);
    tty.c_cflag &= ~(CSTOPB | CRTSCTS);
    tty.c_cflag |= CLOCAL | CREAD;
    tcflush(uart_fd, TCIFLUSH);
    tcsetattr(uart_fd, TCSANOW, &tty);
  }

  if((i2c_fd = open("/dev/i2c-0", O_RDWR)) >= 0)
  {
    if(ioctl(i2c_fd, I2C_SLAVE_FORCE, ADDR_PENE) >= 0)
    {
      /* set all pins to low */
      if(i2c_write_addr_data16(i2c_fd, 0x02, 0x0000) > 0)
      {
        i2c_pene = 1;
        /* configure all pins as output */
        i2c_write_addr_data16(i2c_fd, 0x06, 0x0000);
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_ALEX) >= 0)
    {
      /* set all pins to low */
      if(i2c_write_addr_data16(i2c_fd, 0x02, 0x0000) > 0)
      {
        i2c_alex = 1;
        /* configure all pins as output */
        i2c_write_addr_data16(i2c_fd, 0x06, 0x0000);
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_LEVEL) >= 0)
    {
      /* set all pins to low */
      if(i2c_write_addr_data16(i2c_fd, 0x02, 0x0000) > 0)
      {
        i2c_level = 1;
        /* configure all pins as output */
        i2c_write_addr_data16(i2c_fd, 0x06, 0x0000);
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_MISC) >= 0)
    {
      /* set all pins to low */
      if(i2c_write_addr_data16(i2c_fd, 0x02, 0x0000) > 0)
      {
        i2c_misc = 1;
        /* configure all pins as output */
        i2c_write_addr_data16(i2c_fd, 0x06, 0x0000);
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_DRIVE) >= 0)
    {
      /* set both potentiometers to 0 */
      if(i2c_write_addr_data16(i2c_fd, 0xa9, 0x0000) > 0)
      {
        i2c_drive = 1;
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC0) >= 0)
    {
      if(i2c_write_data16(i2c_fd, i2c_dac0_data) > 0)
      {
        i2c_dac0 = 1;
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC1) >= 0)
    {
      if(i2c_write_data16(i2c_fd, i2c_dac1_data) > 0)
      {
        i2c_dac1 = 1;
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO) >= 0)
    {
      if(i2c_write_addr_data16(i2c_fd, 0x1, i2c_ard_frx1_data) > 0)
      {
        i2c_arduino = 1;
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_NUCLEO) >= 0)
    {
      if(i2c_write_data40(i2c_fd, 0x02100000, 0) > 0)
      {
        i2c_nucleo = 1;
      }
    }
    if(ioctl(i2c_fd, I2C_SLAVE, ADDR_CODEC) >= 0)
    {
      /* reset */
      if(i2c_write_addr_data8(i2c_fd, 0x1e, 0x00) > 0)
      {
        i2c_codec = 1;
        /* set power down register */
        i2c_write_addr_data8(i2c_fd, 0x0c, 0x51);
        /* reset activate register */
        i2c_write_addr_data8(i2c_fd, 0x12, 0x00);
        /* set volume to -10 dB */
        i2c_write_addr_data8(i2c_fd, 0x04, 0x6f);
        i2c_write_addr_data8(i2c_fd, 0x06, 0x6f);
        /* set analog audio path register */
        i2c_write_addr_data8(i2c_fd, 0x08, 0x14);
        /* set digital audio path register */
        i2c_write_addr_data8(i2c_fd, 0x0a, 0x00);
        /* set format register */
        i2c_write_addr_data8(i2c_fd, 0x0e, 0x42);
        /* set activate register */
        i2c_write_addr_data8(i2c_fd, 0x12, 0x01);
        /* set power down register */
        i2c_write_addr_data8(i2c_fd, 0x0c, 0x41);
      }
    }
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x41000000);
  fifo = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x42000000);
  codec = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x43000000);
  xadc = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x44000000);
  alex = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x45000000);
  tx_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x46000000);
  dac_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x47000000);

  rx_rst = (uint8_t *)(cfg + 0);
  lo_rst = (uint8_t *)(cfg + 1);
  tx_rst = (uint8_t *)(cfg + 2);
  gpio_out = (uint8_t *)(cfg + 3);

  rx_rate = (uint16_t *)(cfg + 4);

  rx_sel = (uint8_t *)(cfg + 6);

  rx_freq = (uint32_t *)(cfg + 8);

  tx_freq = (uint32_t *)(cfg + 20);
  tx_size = (uint16_t *)(cfg + 24);
  tx_level = (int16_t *)(cfg + 26);
  ps_level = (int16_t *)(cfg + 28);

  tx_sel = (uint8_t *)(cfg + 30);

  dac_freq = (uint32_t *)(cfg + 32);
  dac_size = (uint16_t *)(cfg + 36);
  dac_level = (int16_t *)(cfg + 38);

  rx_cntr = (uint16_t *)(sts + 0);
  tx_cntr = (uint16_t *)(sts + 2);
  dac_cntr = (uint16_t *)(sts + 4);
  adc_cntr = (uint16_t *)(sts + 6);
  gpio_in = (uint8_t *)(sts + 8);

  /* set rx and tx selectors */
  *rx_sel = chan & 15;
  *tx_sel = (chan >> 4) & 3;

  /* set all GPIO pins to low */
  *gpio_out = 0;

  /* set default rx phase increment */
  rx_freq[0] = (uint32_t)floor(600000 / 122.88e6 * (1 << 30) + 0.5);
  rx_freq[1] = (uint32_t)floor(600000 / 122.88e6 * (1 << 30) + 0.5);
  rx_freq[2] = (uint32_t)floor(600000 / 122.88e6 * (1 << 30) + 0.5);

  /* set default rx sample rate */
  *rx_rate = 1280;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000 / 122.88e6 * (1 << 30) + 0.5);

  /* set tx ramp */
  size = 1001;
  ramp[0] = 0.0;
  for(i = 1; i <= size; ++i)
  {
    ramp[i] = ramp[i - 1] + a[0] - a[1] * cos(2.0 * M_PI * i / size) + a[2] * cos(4.0 * M_PI * i / size) - a[3] * cos(6.0 * M_PI * i / size);
  }
  scale = 4.1e6 / ramp[size];
  for(i = 0; i <= size; ++i)
  {
    tx_ramp[i] = (int32_t)floor(ramp[i] * scale + 0.5);
  }
  *tx_size = size;

  /* set default tx level */
  *tx_level = 21910;

  /* set ps level */
  *ps_level = 18716;

  /* set default tx mux channel */
  *tx_rst &= ~16;

  /* reset tx and codec DAC fifo */
  *tx_rst &= ~3;
  *tx_rst |= 3;

  /* reset tx lo */
  *lo_rst &= ~4;
  *lo_rst |= 4;

  if(i2c_codec)
  {
    /* reset codec ADC fifo */
    *rx_rst &= ~2;
    *rx_rst |= 2;
    /* enable I2S interface */
    *rx_rst &= ~4;

    /* set default dac phase increment */
    *dac_freq = (uint32_t)floor(600 / 48.0e3 * (1 << 30) + 0.5);

    /* set dac ramp */
    size = 481;
    ramp[0] = 0.0;
    for(i = 1; i <= size; ++i)
    {
      ramp[i] = ramp[i - 1] + a[0] - a[1] * cos(2.0 * M_PI * i / size) + a[2] * cos(4.0 * M_PI * i / size) - a[3] * cos(6.0 * M_PI * i / size);
    }
    scale = 3.2e4 / ramp[size];
    for(i = 0; i <= size; ++i)
    {
      dac_ramp[i] = (int16_t)floor(ramp[i] * scale + 0.5);
    }
    *dac_size = size;

    /* set default dac level */
    *dac_level = 3200;
  }
  else
  {
    /* enable ALEX interface */
    *rx_rst |= 4;
  }

  calc_log_lookup();

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

  tv.tv_sec = 0;
  tv.tv_usec = 1000;
  setsockopt(sock_ep2, SOL_SOCKET, SO_RCVTIMEO, (void *)&tv , sizeof(tv));

  memset(&addr_ep2, 0, sizeof(addr_ep2));
  addr_ep2.sin_family = AF_INET;
  addr_ep2.sin_addr.s_addr = htonl(INADDR_ANY);
  addr_ep2.sin_port = htons(1024);

  if(bind(sock_ep2, (struct sockaddr *)&addr_ep2, sizeof(addr_ep2)) < 0)
  {
    perror("bind");
    return EXIT_FAILURE;
  }

  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
  param.sched_priority = 99;
  pthread_attr_setschedparam(&attr, &param);
  if(pthread_create(&thread, &attr, handler_keyer, NULL) < 0)
  {
    perror("pthread_create");
    return EXIT_FAILURE;
  }
  pthread_detach(thread);

  while(1)
  {
    memset(iovec, 0, sizeof(iovec));
    memset(datagram, 0, sizeof(datagram));

    for(i = 0; i < 8; ++i)
    {
      memcpy(buffer[i], id, 4);
      iovec[i][0].iov_base = buffer[i];
      iovec[i][0].iov_len = 1032;
      datagram[i].msg_hdr.msg_iov = iovec[i];
      datagram[i].msg_hdr.msg_iovlen = 1;
      datagram[i].msg_hdr.msg_name = &addr_from[i];
      datagram[i].msg_hdr.msg_namelen = sizeof(addr_from[i]);
    }

    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;

    size = recvmmsg(sock_ep2, datagram, 8, 0, &ts);
    if(size < 0 && errno != EAGAIN)
    {
      perror("recvfrom");
      return EXIT_FAILURE;
    }

    for(i = 0; i < size; ++i)
    {
      memcpy(&code, buffer[i], 4);
      switch(code)
      {
        case 0x0201feef:
          if(!tx_mux_data)
          {
            while(*tx_cntr > 3844) usleep(1000);
            if(*tx_cntr == 0) for(j = 0; j < 2520; ++j) *fifo = 0;
            if((*gpio_out & 1) | (*gpio_in & 1))
            {
              for(j = 0; j < 504; j += 8)
              {
                *fifo = tx_eer_data ? *(uint32_t *)(buffer[i] + 16 + j) : 0;
                *fifo = *(uint32_t *)(buffer[i] + 20 + j);
              }
              for(j = 0; j < 504; j += 8)
              {
                *fifo = tx_eer_data ? *(uint32_t *)(buffer[i] + 528 + j) : 0;
                *fifo = *(uint32_t *)(buffer[i] + 532 + j);
              }
            }
            else
            {
              for(j = 0; j < 252; ++j) *fifo = 0;
            }
          }
          if(i2c_codec)
          {
            while(*dac_cntr > 1922) usleep(1000);
            if(*dac_cntr == 0) for(j = 0; j < 1260; ++j) *codec = 0;
            for(j = 0; j < 504; j += 8) *codec = *(uint32_t *)(buffer[i] + 16 + j);
            for(j = 0; j < 504; j += 8) *codec = *(uint32_t *)(buffer[i] + 528 + j);
          }
          process_ep2(buffer[i] + 11);
          process_ep2(buffer[i] + 523);
          break;
        case 0x0002feef:
          reply[2] = 2 + active_thread;
          memset(buffer[i], 0, 60);
          memcpy(buffer[i], reply, 11);
          sendto(sock_ep2, buffer[i], 60, 0, (struct sockaddr *)&addr_from[i], sizeof(addr_from[i]));
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
          addr_ep6.sin_addr.s_addr = addr_from[i].sin_addr.s_addr;
          addr_ep6.sin_port = addr_from[i].sin_port;
          enable_thread = 1;
          active_thread = 1;
          rx_sync_data = 0;
          /* reset rx los */
          *lo_rst &= ~3;
          *lo_rst |= 3;
          if(pthread_create(&thread, NULL, handler_ep6, NULL) < 0)
          {
            perror("pthread_create");
            return EXIT_FAILURE;
          }
          pthread_detach(thread);
          break;
      }
    }

    data = alex_data_0 | cw_ptt;
    if(alex_data_0 != data)
    {
      alex_data_0 = data;
      alex_update = 1;
    }

    if(alex_update)
    {
      alex_update = 0;
      alex_write();
    };

    if(misc_update)
    {
      misc_update = 0;
      misc_write();
    };

    data = nucleo_data_0 | cw_ptt;
    if(nucleo_data_0 != data)
    {
      nucleo_data_0 = data;
      nucleo_update = 1;
    }

    if(nucleo_update)
    {
      nucleo_update = 0;
      nucleo_write();
    };
  }
  close(sock_ep2);

  return EXIT_SUCCESS;
}

void process_ep2(uint8_t *frame)
{
  uint32_t freq;
  uint16_t data;
  uint32_t data32;
  uint8_t ptt, preamp, att, boost;
  uint8_t data8;

  switch(frame[0])
  {
    case 0:
    case 1:
      receivers = ((frame[4] >> 3) & 7) + 1;
      data = (frame[4] >> 7) & 1;
      if(rx_sync_data != data)
      {
        rx_sync_data = data;
        if(rx_sync_data)
        {
          rx_freq[1] = rx_freq[0];
          /* reset rx los */
          *lo_rst &= ~3;
          *lo_rst |= 3;
        }
      }
      tx_eer_data = frame[2] & 1;

      /* set output pins */
      ptt = frame[0] & 0x01;
      att = frame[3] & 0x03;
      preamp = ptt | (*gpio_in & 1) ? 0 : (frame[3] & 0x04) >> 2 | (rx_att_data == 0);
      *gpio_out = (frame[2] & 0x1e) << 3 | att << 2 | preamp << 1 | ptt;

      /* set rx sample rate */
      rate = frame[1] & 3;
      switch(frame[1] & 3)
      {
        case 0:
          *rx_rate = 1280;
          break;
        case 1:
          *rx_rate = 640;
          break;
        case 2:
          *rx_rate = 320;
          break;
        case 3:
          *rx_rate = 160;
          break;
      }

      data = (frame[4] & 0x03) << 8 | (frame[3] & 0xe0) | (frame[3] & 0x03) << 1 | (frame[0] & 0x01);
      if(alex_data_0 != data)
      {
        alex_data_0 = data;
        alex_update = 1;
      }

      /* configure PENELOPE */
      if(i2c_pene)
      {
        data = (frame[3] & 0x1c) << 11 | (frame[4] & 0x03) << 11 | (frame[3] & 0x60) << 4 | (frame[3] & 0x03) << 7 | frame[2] >> 1;
        if(i2c_pene_data != data)
        {
          i2c_pene_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_PENE);
          i2c_write_addr_data16(i2c_fd, 0x02, data);
        }
      }

      if(i2c_nucleo)
      {
        data =
          ((frame[4] & 0x03) == 2) << 15 |
          ((frame[4] & 0x03) == 1) << 14 |
          ((frame[4] & 0x03) == 0) << 13 |
          (((frame[3] >> 5) & 0x03) == 3) << 12 |
          (((frame[3] >> 5) & 0x03) == 2) << 11 |
          (((frame[3] >> 5) & 0x03) == 1) << 10 |
          (frame[3] & 0x18) << 5 | (frame[2] & 0xfe) | (frame[0] & 0x01);

        if(nucleo_data_0 != data)
        {
          nucleo_data_0 = data;
          nucleo_update = 1;
        }
      }

      if(i2c_dac0)
      {
        data = rx_att_data + 10 * att;
        data = 4065 - (uint32_t)data * 4095 / 61;
        if(i2c_dac0_data != data)
        {
          i2c_dac0_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_DAC0);
          i2c_write_data16(i2c_fd, data);
        }
      }

      if(i2c_arduino)
      {
        /*
        24 bit data field: 0RRR00TT0XXXXXXX0YYYYYYY
        RRR=RX ant
        TT=TX ant
        XXXXXXX=RX OC
        YYYYYYY=TX OC
        */
        if(frame[0] & 0x01)
        {
          data32 = i2c_ard_ocant_data & 0x00fc7f00;
          data32 |= (frame[2] >> 1); /* add back in OC bits */
          data32 |= (frame[4] & 0x03) << 16;  /* add back TX ant bits */
        }
        else
        {
          data32 = i2c_ard_ocant_data & 0x0003007f;
          data32 |= (frame[2] << 7); /* add back in OC bits */
          data8 = (frame[3] & 0x60) >> 5; /* RX aux bits */
          if(data8 == 0)
          {
            data32 |= (frame[4] & 0x03) << 20; /* use TX bit positions */
          }
          else
          {
            data8 += 2;
            data32 |= (data8 & 0x07) << 20; /* RX bit positions */
          }
        }
        if(data32 != i2c_ard_ocant_data)
        {
          i2c_ard_ocant_data = data32;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
          i2c_write_addr_data24(i2c_fd, 0x4, data32);
        }
      }
      break;
    case 2:
    case 3:
      /* set tx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *tx_freq = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      if(freq_data[0] != freq)
      {
        freq_data[0] = freq;
        icom_write();
        alex_update = 1;
        if(i2c_misc) misc_update = 1;
        if(i2c_nucleo) nucleo_update = 1;
        if(i2c_arduino)
        {
          data = freq / 1000;
          if(data != i2c_ard_ftx_data)
          {
            i2c_ard_ftx_data = data;
            ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
            i2c_write_addr_data16(i2c_fd, 0x03, data);
          }
        }
      }
      break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[0] = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      if(rx_sync_data) rx_freq[1] = rx_freq[0];
      if(freq_data[1] != freq)
      {
        freq_data[1] = freq;
        if(rx_sync_data)
        {
          /* reset rx los */
          *lo_rst &= ~3;
          *lo_rst |= 3;
        }
        alex_update = 1;
        if(i2c_misc) misc_update = 1;
        if(i2c_nucleo) nucleo_update = 1;
        if(i2c_arduino)
        {
          data = freq / 1000;
          if(data != i2c_ard_frx1_data)
          {
            i2c_ard_frx1_data = data;
            ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
            i2c_write_addr_data16(i2c_fd, 0x01, data);
          }
        }
      }
      break;
#ifndef THETIS
    case 6:
    case 7:
      /* set rx phase increment */
      if(rx_sync_data) break;
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[1] = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      if(freq_data[2] != freq)
      {
        freq_data[2] = freq;
        alex_update = 1;
        if(i2c_misc) misc_update = 1;
        if(i2c_nucleo) nucleo_update = 1;
        if(i2c_arduino)
        {
          data = freq / 1000;
          if(data != i2c_ard_frx2_data)
          {
            i2c_ard_frx2_data = data;
            ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
            i2c_write_addr_data16(i2c_fd, 0x02, data);
          }
        }
      }
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[2] = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      break;
#else
    case 6:
    case 7:
      /* set rx phase increment */
      if(rx_sync_data) break;
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[1] = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      rx_freq[2] = (uint32_t)floor(freq / 122.88e6 * (1 << 30) + 0.5);
      if(freq_data[2] != freq)
      {
        freq_data[2] = freq;
        alex_update = 1;
        if(i2c_misc) misc_update = 1;
        if(i2c_nucleo) nucleo_update = 1;
        if(i2c_arduino)
        {
          data = freq / 1000;
          if(data != i2c_ard_frx2_data)
          {
            i2c_ard_frx2_data = data;
            ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
            i2c_write_addr_data16(i2c_fd, 0x02, data);
          }
        }
      }
      break;
#endif
    case 18:
    case 19:
      data = (frame[2] & 0x40) << 9 | frame[4] << 8 | frame[3];
      if(alex_data_1 != data)
      {
        alex_data_1 = data;
        alex_update = 1;
      }

      if(i2c_misc)
      {
        data = (frame[3] & 0x80) >> 6 | (frame[3] & 0x20) >> 5;
        if(misc_data_2 != data)
        {
          misc_data_2 = data;
          misc_update = 1;
        }
      }

      if(i2c_nucleo)
      {
        data = (frame[3] & 0xe0) >> 5;
        if(nucleo_data_1 != data)
        {
          nucleo_data_1 = data;
          nucleo_update = 1;
        }
      }

      /* configure ALEX */
      if(i2c_alex)
      {
        data = frame[4] << 8 | frame[3];
        if(i2c_alex_data != data)
        {
          i2c_alex_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ALEX);
          i2c_write_addr_data16(i2c_fd, 0x02, data);
        }
      }

      /* configure level */
      data = frame[1];
      if(i2c_level)
      {
        if(i2c_level_data != data)
        {
          i2c_level_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_LEVEL);
          i2c_write_addr_data16(i2c_fd, 0x02, data);
        }
      }
      else if(i2c_drive)
      {
        if(i2c_drive_data != data)
        {
          i2c_drive_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_DRIVE);
          i2c_write_addr_data16(i2c_fd, 0xa9, data << 8 | data);
        }
      }
      else if(i2c_arduino)
      {
        /*
        24 bit data field 000000BA000RRRRR00TTTTTT
        BA=PA disable, 6m LNA
        RRRRR=5 bits RX att when in TX
        TTTTTT=6 bits TX att (0.5 dB units)
        */
        data32 = i2c_ard_txatt_data & 0x00001f00; /* remove TX att */
        data32 |= log_table_lookup[data];
        data32 |= (frame[3] & 0xc0) << 10;
        if(data32 != i2c_ard_txatt_data)
        {
          i2c_ard_txatt_data = data32;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
          i2c_write_addr_data24(i2c_fd, 0x05, data32);
        }
      }
      else
      {
        *tx_level = (int16_t)floor(data * 85.920 + 0.5);
      }
      /* configure microphone boost */
      if(i2c_codec)
      {
        boost = frame[2] & 0x01;
        if(i2c_boost_data != boost)
        {
          i2c_boost_data = boost;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_CODEC);
          i2c_write_addr_data8(i2c_fd, 0x08, 0x14 + boost);
        }
      }
      break;
    case 20:
    case 21:
      rx_att_data = frame[4] & 0x1f;
      if(i2c_misc)
      {
        data = frame[4] & 0x1f;
        if(misc_data_0 != data)
        {
          misc_data_0 = data;
          misc_update = 1;
        }
      }
      if(i2c_nucleo)
      {
        data = frame[4] & 0x1f;
        if(nucleo_data_2 != data)
        {
          nucleo_data_2 = data;
          nucleo_update = 1;
        }
      }
      if(i2c_arduino)
      {
        /*
        16 bit data field 000RRRRR000TTTTT
        RRRRR=RX2
        TTTTT=RX1
        */
        data = i2c_ard_rxatt_data & 0x00001f00; /* remove RX1 att */
        data |= (frame[4] & 0x1f);
        if(data != i2c_ard_rxatt_data)
        {
          i2c_ard_rxatt_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
          i2c_write_addr_data16(i2c_fd, 0x06, data);
        }
      }
      break;
    case 22:
    case 23:
      if(i2c_misc)
      {
        data = frame[1] & 0x1f;
        if(misc_data_1 != data)
        {
          misc_data_1 = data;
          misc_update = 1;
        }
      }
      if(i2c_nucleo)
      {
        data = frame[1] & 0x1f;
        if(nucleo_data_3 != data)
        {
          nucleo_data_3 = data;
          nucleo_update = 1;
        }
      }
      cw_reversed = (frame[2] >> 6) & 1;
      cw_speed = frame[3] & 63;
      cw_mode = (frame[3] >> 6) & 3;
      cw_weight = frame[4] & 127;
      cw_spacing = (frame[4] >> 7) & 1;

      if(i2c_arduino)
      {
        /*
        16 bit data field 000RRRRR000TTTTT
        RRRRR=RX2;
        TTTTT=RX1
        */
        data = i2c_ard_rxatt_data & 0x1f; /* remove RX2 att */
        data |= (frame[1] & 0x1f) << 8;
        if(data != i2c_ard_rxatt_data)
        {
          i2c_ard_rxatt_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
          i2c_write_addr_data16(i2c_fd, 0x06, data);
        }
      }
      break;
    case 28:
    case 29:
      if(i2c_arduino)
      {
        /*
        24 bit data field 000000BA000RRRRR00TTTTTT
        BA=PA disable, 6m LNA
        RRRRR=5 bits RX att when in TX
        TTTTTT=6 bits TX att (0.5 dB units)
        */
        data32 = i2c_ard_txatt_data & 0x0003003f; /* remove RX att */
        data32 |= (frame[3] & 0x1f) << 8;
        if(data32 != i2c_ard_txatt_data)
        {
          i2c_ard_txatt_data = data32;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_ARDUINO);
          i2c_write_addr_data24(i2c_fd, 0x05, data32);
        }
      }
      break;
    case 30:
    case 31:
      cw_int_data = frame[1] & 1;
      dac_level_data = frame[2];
      cw_delay = frame[3];
      if(i2c_codec)
      {
        data = dac_level_data;
        *dac_level = data * 64;
      }
      break;
    case 32:
    case 33:
      cw_hang = (frame[1] << 2) | (frame[2] & 3);
      if(i2c_codec)
      {
        freq = (frame[3] << 4) | (frame[4] & 255);
        *dac_freq = (uint32_t)floor(freq / 48.0e3 * (1 << 30) + 0.5);
      }
      break;
  }
}

void *handler_ep6(void *arg)
{
  int i, j, k, m, n, size, rate_counter;
  int data_offset, header_offset;
  uint32_t counter;
  uint16_t value;
  uint32_t audio[512];
  uint8_t data[4 * 4096];
  uint8_t buffer[25 * 1032];
  uint8_t *pointer;
  struct iovec iovec[25][1];
  struct mmsghdr datagram[25];
  uint8_t id[4] = {0xef, 0xfe, 1, 6};
  uint8_t header[40] =
  {
    127, 127, 127, 0, 0, 33, 17, 21,
    127, 127, 127, 8, 0, 0, 0, 0,
    127, 127, 127, 16, 0, 0, 0, 0,
    127, 127, 127, 24, 0, 0, 0, 0,
    127, 127, 127, 32, 66, 66, 66, 66
  };

  memset(audio, 0, sizeof(audio));
  memset(iovec, 0, sizeof(iovec));
  memset(datagram, 0, sizeof(datagram));

  for(i = 0; i < 25; ++i)
  {
    memcpy(buffer + i * 1032, id, 4);
    iovec[i][0].iov_base = buffer + i * 1032;
    iovec[i][0].iov_len = 1032;
    datagram[i].msg_hdr.msg_iov = iovec[i];
    datagram[i].msg_hdr.msg_iovlen = 1;
    datagram[i].msg_hdr.msg_name = &addr_ep6;
    datagram[i].msg_hdr.msg_namelen = sizeof(addr_ep6);
  }

  header_offset = 0;
  counter = 0;
  rate_counter = 1 << rate;
  k = 0;

  if(i2c_codec)
  {
    /* reset codec ADC fifo */
    *rx_rst &= ~2;
    *rx_rst |= 2;
  }

  /* reset rx fifo */
  *rx_rst &= ~1;
  *rx_rst |= 1;

  while(1)
  {
    if(!enable_thread) break;

    size = receivers * 6 + 2;
    n = 504 / size;
    m = 256 / n;

    if((i2c_codec && *adc_cntr >= 1024) || *rx_cntr >= 8192)
    {
      if(i2c_codec)
      {
        /* reset codec ADC fifo */
        *rx_rst &= ~2;
        *rx_rst |= 2;
      }

      /* reset rx fifo */
      *rx_rst &= ~1;
      *rx_rst |= 1;
    }

    while(*rx_cntr < m * n * 16) usleep(1000);

    if(i2c_codec && --rate_counter == 0)
    {
      memcpy(audio, codec, m * n * 8);
      rate_counter = 1 << rate;
      k = 0;
    }

    memcpy(data, fifo, m * n * 64);

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
      pointer[3] |= (*gpio_in & 7) | cw_ptt;
      if(header_offset == 8)
      {
        value = xadc[24] >> 3;
        pointer[6] = (value >> 8) & 0xff;
        pointer[7] = value & 0xff;
      }
      else if(header_offset == 16)
      {
        value = xadc[16] >> 3;
        pointer[4] = (value >> 8) & 0xff;
        pointer[5] = value & 0xff;
        value = xadc[17] >> 3;
        pointer[6] = (value >> 8) & 0xff;
        pointer[7] = value & 0xff;
      }
      else if(header_offset == 24)
      {
        value = xadc[25] >> 3;
        pointer[4] = (value >> 8) & 0xff;
        pointer[5] = value & 0xff;
      }
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;

      pointer += 8;
      memset(pointer, 0, 504);
      for(j = 0; j < n; ++j)
      {
        memcpy(pointer, data + data_offset, size > 14 ? 12 : size - 2);
        if(size > 14)
        {
#ifndef THETIS
          memcpy(pointer + 12, data + 18 + data_offset, size > 26 ? 12 : size - 14);
#else
          memcpy(pointer + 12, data + 12 + data_offset, size > 32 ? 18 : size - 14);
#endif
        }
        data_offset += 32;
        pointer += size;
        if(i2c_codec) memcpy(pointer - 2, &audio[(k++) >> rate], 2);
      }
    }

    sendmmsg(sock_ep2, datagram, m, 0);
  }

  active_thread = 0;

  return NULL;
}

static inline int cw_input()
{
  int input;
  if(!cw_int_data) return 0;
  input = (*gpio_in >> 1) & 3;
  if(cw_reversed) input = (input & 1) << 1 | input >> 1;
  return input;
}

static inline void cw_on()
{
  int delay = 1200 / cw_speed;
  if(cw_delay < delay) delay = cw_delay;
  /* PTT on */
  *tx_rst |= 32;
  cw_ptt = 1;
  *tx_rst |= 16;
  tx_mux_data = 1;
  if(i2c_codec && dac_level_data > 0)
  {
    *tx_rst |= 8; /* sidetone on */
  }
  while(delay--)
  {
    usleep(1000);
    cw_memory[0] = cw_input();
    if(cw_mode == 1 && !cw_memory[0]) cw_memory[1] = 0;
    else cw_memory[1] |= cw_memory[0];
  }
  *tx_rst |= 4; /* RF on */
}

static inline void cw_off()
{
  int delay = 1200 / cw_speed;
  if(cw_delay < delay) delay = cw_delay;
  if(i2c_codec)
  {
    *tx_rst &= ~8; /* sidetone off */
  }
  while(delay--)
  {
    usleep(1000);
    cw_memory[0] = cw_input();
    cw_memory[1] |= cw_memory[0];
  }
  *tx_rst &= ~4; /* RF off */
  cw_ptt_delay = cw_hang > 0 ? cw_hang : 10;
}

static inline void cw_ptt_off()
{
  if(--cw_ptt_delay > 0) return;
  /* PTT off */
  *tx_rst &= ~32;
  cw_ptt = 0;
  /* reset tx fifo */
  *tx_rst &= ~1;
  *tx_rst |= 1;
  *tx_rst &= ~16;
  tx_mux_data = 0;
}

static inline void cw_signal_delay(int code)
{
  int delay = code ? 1200 / cw_speed : 3600 * cw_weight / (50 * cw_speed);
  delay -= cw_delay;
  if(delay < 0) delay = 0;
  while(delay--)
  {
    usleep(1000);
    cw_memory[0] = cw_input();
    if(cw_mode == 1 && !cw_memory[0]) cw_memory[1] = 0;
    else cw_memory[1] |= cw_memory[0];
  }
}

static inline void cw_space_delay(int code)
{
  int delay = code ? 1200 / cw_speed - cw_delay : 2400 / cw_speed;
  if(delay < 0) delay = 0;
  while(delay--)
  {
    usleep(1000);
    if(cw_ptt) cw_ptt_off();
    cw_memory[0] = cw_input();
    cw_memory[1] |= cw_memory[0];
  }
}

void *handler_keyer(void *arg)
{
  int state, delay;

  while(1)
  {
    usleep(1000);
    if(cw_ptt) cw_ptt_off();
    if(!(cw_memory[0] = cw_input())) continue;

    if(cw_mode == 0)
    {
      if(cw_memory[0] & 1)
      {
        cw_on();
        while(cw_memory[0] & 1)
        {
          usleep(1000);
          cw_memory[0] = cw_input();
        }
        cw_off();
      }
      else
      {
        cw_on();
        delay = 1200 / cw_speed - cw_delay;
        if(delay > 0) usleep(delay * 1000);
        cw_off();
        cw_space_delay(1);
      }
    }
    else
    {
      state = 1;
      cw_memory[1] = cw_memory[0];
      while(1)
      {
        if(cw_memory[1] & (1 << state))
        {
          cw_memory[1] = 0;
          cw_on();
          cw_signal_delay(state);
          cw_off();
          cw_space_delay(1);
          cw_memory[1] &= ~(1 << state);
          cw_memory[1] |= cw_memory[0];
        }
        if(cw_memory[1])
        {
          state ^= 1;
        }
        else
        {
          if(cw_spacing)
          {
            state = 1;
            cw_space_delay(0);
            if(cw_memory[1]) continue;
          }
          break;
        }
      }
    }
  }

  return NULL;
}
