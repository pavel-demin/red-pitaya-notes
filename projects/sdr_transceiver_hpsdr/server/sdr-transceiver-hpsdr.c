/*
19.04.2016 DC2PD: add code for bandpass and antenna switching via I2C.
22.08.2016 DL4AOI: add code for TX level switching via I2C.
22.08.2016 DL4AOI: output first four open collector outputs to the pins DIO4_P - DIO7_P of the extension connector E1.
02.09.2016 ON3VNA: add code for TX level switching via DS1803-10 (I2C).
21.09.2016 DC2PD: add code for controlling AD8331 VGA with MCP4725 DAC (I2C).
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
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "jack/ringbuffer.c"

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_PENE 0x20 /* PCA9555 address 0 */
#define ADDR_ALEX 0x21 /* PCA9555 address 1 */
#define ADDR_LEVEL 0x22 /* PCA9555 address 2 */
#define ADDR_DRIVE 0x28 /* DS1803 address 0 */
#define ADDR_CODEC 0x1A /* WM8731 or TLV320AIC23B address 0 */
#define ADDR_DAC0 0x60 /* MCP4725 address 0 */
#define ADDR_DAC1 0x61 /* MCP4725 address 1 */

volatile uint32_t *rx_freq[4], *rx_rate, *tx_freq, *alex, *tx_mux, *dac_freq, *dac_mux;
volatile uint16_t *rx_cntr, *tx_cntr, *tx_level, *dac_cntr, *dac_level, *adc_cntr;
volatile uint8_t *gpio_in, *gpio_out, *rx_rst, *tx_rst, *codec_rst;
volatile uint64_t *rx_data;
volatile uint32_t *tx_data, *dac_data;
volatile uint16_t *adc_data;
volatile int32_t *xadc;

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
void *handler_playback(void *arg);

jack_ringbuffer_t *playback_data = 0;

/* variables to handle I2C devices */
int i2c_fd;
int i2c_pene = 0;
int i2c_alex = 0;
int i2c_level = 0;
int i2c_drive = 0;
int i2c_codec = 0;
int i2c_dac0 = 0;
int i2c_dac1 = 0;

uint16_t i2c_pene_data = 0;
uint16_t i2c_alex_data = 0;
uint16_t i2c_level_data = 0;
uint16_t i2c_drive_data = 0;
uint16_t i2c_dac0_data = 0xfff;
uint16_t i2c_dac1_data = 0xfff;

uint8_t i2c_boost_data = 0;

uint8_t cw_int_data = 0;
uint8_t cw_mux_data = 0;
uint8_t rx_att_data = 0;

ssize_t i2c_write_addr_data8(int fd, uint8_t addr, uint8_t data)
{
  uint8_t buffer[2];
  ssize_t size;
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

ssize_t i2c_write_data16(int fd, uint16_t data)
{
  uint8_t buffer[2];
  buffer[0] = data >> 8;
  buffer[1] = data;
  return write(fd, buffer, 2);
}

uint16_t alex_data_rx = 0;
uint16_t alex_data_tx = 0;
uint16_t alex_data_0 = 0;
uint32_t alex_data_1 = 0;
uint32_t alex_data_2 = 0;
uint32_t alex_data_3 = 0;
uint16_t alex_data_4 = 0;

void alex_write()
{
  uint32_t max = alex_data_2 > alex_data_3 ? alex_data_2 : alex_data_3;
  uint16_t manual = (alex_data_4 >> 15) & 0x01;
  uint16_t preamp = manual ? (alex_data_4 >> 6) & 0x01 : max > 50000000;
  uint16_t ptt = alex_data_0 & 0x01;
  uint32_t freq = 0;
  uint16_t hpf = 0, lpf = 0, data = 0;

  if(i2c_codec) return;

  freq = alex_data_2 < alex_data_3 ? alex_data_2 : alex_data_3;

  if(preamp) hpf = 0;
  else if(manual) hpf = alex_data_4 & 0x3f;
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

  freq = ptt ? alex_data_1 : max;

  if(manual) lpf = (alex_data_4 >> 8) & 0x7f;
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
    (~(alex_data_4 >> 7) & ptt) << 11 |
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

int main(int argc, char *argv[])
{
  int fd, i, j, size;
  pthread_t thread;
  volatile void *cfg, *sts;
  volatile int32_t *tx_ramp, *dac_ramp;
  volatile uint16_t *tx_size, *dac_size;
  float scale, ramp[2048], a[4] = {0.35875, 0.48829, 0.14128, 0.01168};
  uint8_t reply[11] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 21, 0};
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from[10];
  uint8_t buffer[10][1032];
  struct iovec iovec[10][1];
  struct mmsghdr datagram[10];
  struct timespec timeout;
  int yes = 1;

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
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
        /* set volume to 0 dB */
        i2c_write_addr_data8(i2c_fd, 0x04, 0x79);
        i2c_write_addr_data8(i2c_fd, 0x06, 0x79);
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

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  alex = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  tx_mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);
  tx_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
  dac_mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40005000);
  dac_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
  dac_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);
  adc_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
  tx_data = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000a000);
  rx_data = mmap(NULL, 8*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);
  xadc = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  codec_rst = ((uint8_t *)(cfg + 2));
  gpio_out = ((uint8_t *)(cfg + 3));

  rx_rate = ((uint32_t *)(cfg + 4));

  rx_freq[0] = ((uint32_t *)(cfg + 8));
  rx_freq[1] = ((uint32_t *)(cfg + 12));
  rx_freq[2] = ((uint32_t *)(cfg + 16));
  rx_freq[3] = ((uint32_t *)(cfg + 20));

  tx_freq = ((uint32_t *)(cfg + 24));
  tx_size = ((uint16_t *)(cfg + 28));
  tx_level = ((uint16_t *)(cfg + 30));

  dac_freq = ((uint32_t *)(cfg + 32));
  dac_size = ((uint16_t *)(cfg + 36));
  dac_level = ((uint16_t *)(cfg + 38));

  rx_cntr = ((uint16_t *)(sts + 12));
  tx_cntr = ((uint16_t *)(sts + 14));
  dac_cntr = ((uint16_t *)(sts + 16));
  adc_cntr = ((uint16_t *)(sts + 18));
  gpio_in = ((uint8_t *)(sts + 20));

  /* set all GPIO pins to low */
  *gpio_out = 0;

  /* set default rx phase increment */
  *rx_freq[0] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
  *rx_freq[1] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
  *rx_freq[2] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);
  *rx_freq[3] = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);

  /* set default rx sample rate */
  *rx_rate = 1000;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000 / 125.0e6 * (1 << 30) + 0.5);

  /* set tx ramp */
  size = 1001;
  ramp[0] = 0.0;
  for(i = 1; i <= size; ++i)
  {
    ramp[i] = ramp[i - 1] + a[0] - a[1] * cos(2.0 * M_PI * i / size) + a[2] * cos(4.0 * M_PI * i / size) - a[3] * cos(6.0 * M_PI * i / size);
  }
  scale = 6.1e6 / ramp[size];
  for(i = 0; i <= size; ++i)
  {
    tx_ramp[i] = (int32_t)floor(ramp[i] * scale + 0.5);
  }
  *tx_size = size;

  /* set default tx level */
  *tx_level = 32767;

  /* set default tx mux channel */
  *(tx_mux + 16) = 0;
  *tx_mux = 2;

  /* reset tx fifo */
  *tx_rst |= 1;
  *tx_rst &= ~1;

  if(i2c_codec)
  {
    /* reset codec fifo */
    *codec_rst |= 3;
    *codec_rst &= ~3;
    /* enable I2S interface */
    *codec_rst &= ~4;

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
      dac_ramp[i] = (int32_t)floor(ramp[i] * scale + 0.5);
    }
    *dac_size = size;

    /* set default dac level */
    *dac_level = 32767;

    /* set default dac mux channel */
    *(dac_mux + 16) = 0;
    *dac_mux = 2;
  }
  else
  {
    /* enable ALEX interface */
    *codec_rst |= 4;

    /* create playback thread */
    playback_data = jack_ringbuffer_create(4096);
    if(pthread_create(&thread, NULL, handler_playback, NULL) < 0)
    {
      perror("pthread_create");
      return EXIT_FAILURE;
    }
    pthread_detach(thread);
  }

  if((sock_ep2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    perror("socket");
    return EXIT_FAILURE;
  }

  strncpy(hwaddr.ifr_name, "eth0", IFNAMSIZ);
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
    memset(iovec, 0, sizeof(iovec));
    memset(datagram, 0, sizeof(datagram));

    for(i = 0; i < 10; ++i)
    {
      *(uint32_t *)(buffer[i] + 0) = 0x0601feef;
      iovec[i][0].iov_base = buffer[i];
      iovec[i][0].iov_len = 1032;
      datagram[i].msg_hdr.msg_iov = iovec[i];
      datagram[i].msg_hdr.msg_iovlen = 1;
      datagram[i].msg_hdr.msg_name = &addr_from[i];
      datagram[i].msg_hdr.msg_namelen = sizeof(addr_from[i]);
    }

    timeout.tv_sec = 0;
    timeout.tv_nsec = 1000000;

    size = recvmmsg(sock_ep2, datagram, 10, 0, &timeout);
    if(size < 0)
    {
      perror("recvfrom");
      return EXIT_FAILURE;
    }

    for(i = 0; i < size; ++i)
    {
      switch(*(uint32_t *)buffer[i])
      {
        case 0x0201feef:
          if(!cw_mux_data)
          {
            while(*tx_cntr > 1922) usleep(1000);
            if(*tx_cntr == 0) for(j = 0; j < 1260; ++j) *tx_data = 0;
            if((*gpio_out & 1) | (*gpio_in & 1))
            {
              for(j = 0; j < 504; j += 8) *tx_data = *(uint32_t *)(buffer[i] + 20 + j);
              for(j = 0; j < 504; j += 8) *tx_data = *(uint32_t *)(buffer[i] + 532 + j);
            }
            else
            {
              for(j = 0; j < 126; ++j) *tx_data = 0;
            }
          }
          if(i2c_codec)
          {
            if(!cw_mux_data)
            {
              while(*dac_cntr > 898) usleep(1000);
              if(*dac_cntr == 0) for(j = 0; j < 504; ++j) *dac_data = 0;
              for(j = 0; j < 504; j += 8) *dac_data = *(uint32_t *)(buffer[i] + 16 + j);
              for(j = 0; j < 504; j += 8) *dac_data = *(uint32_t *)(buffer[i] + 528 + j);
            }
          }
          else
          {
            for(j = 0; j < 504; j += 8) jack_ringbuffer_write(playback_data, buffer[i] + 16 + j, 4);
            for(j = 0; j < 504; j += 8) jack_ringbuffer_write(playback_data, buffer[i] + 528 + j, 4);
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
          if(pthread_create(&thread, NULL, handler_ep6, NULL) < 0)
          {
            perror("pthread_create");
            return EXIT_FAILURE;
          }
          pthread_detach(thread);
          break;
      }
    }
  }
  close(sock_ep2);

  return EXIT_SUCCESS;
}

void process_ep2(uint8_t *frame)
{
  uint32_t freq;
  uint16_t data;
  uint16_t cw_hang;
  uint8_t cw_reversed, cw_speed, cw_mode, cw_weight, cw_spacing, cw_delay;
  uint8_t ptt, preamp, att, boost;

  switch(frame[0])
  {
    case 0:
    case 1:
      receivers = ((frame[4] >> 3) & 7) + 1;
      /* set output pins */
      ptt = frame[0] & 0x01;
      att = frame[3] & 0x03;
      preamp = ptt | (*gpio_in & 1) ? 0 : (frame[3] & 0x04) >> 2 | (rx_att_data == 0);
      *gpio_out = (frame[2] & 0x1e) << 3 | att << 2 | preamp << 1 | ptt;

      data =  (ptt | (*gpio_in & 1)) & cw_int_data;
      if(cw_mux_data != data)
      {
        cw_mux_data = data;
        *(tx_mux + 16) = data;
        *tx_mux = 2;
        if(i2c_codec)
        {
          *(dac_mux + 16) = data;
          *dac_mux = 2;
        }
      }

      /* set rx sample rate */
      rate = frame[1] & 3;
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
        case 3:
          *rx_rate = 125;
          break;
      }

      data = (frame[4] & 0x03) << 8 | (frame[3] & 0xe0) | (frame[3] & 0x03) << 1 | (frame[0] & 0x01);
      if(alex_data_0 != data)
      {
        alex_data_0 = data;
        alex_write();
      }

      /* configure PENELOPE */
      if(i2c_pene)
      {
        data = (frame[4] & 0x03) << 11 | (frame[3] & 0x60) << 4 | (frame[3] & 0x03) << 7 | frame[2] >> 1;
        if(i2c_pene_data != data)
        {
          i2c_pene_data = data;
          ioctl(i2c_fd, I2C_SLAVE, ADDR_PENE);
          i2c_write_addr_data16(i2c_fd, 0x02, data);
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
      break;
    case 2:
    case 3:
      /* set tx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(alex_data_1 != freq)
      {
        alex_data_1 = freq;
        alex_write();
      }
      if(freq < freq_min || freq > freq_max) break;
      *tx_freq = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(alex_data_2 != freq)
      {
        alex_data_2 = freq;
        alex_write();
      }
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[0] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 6:
    case 7:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(alex_data_3 != freq)
      {
        alex_data_3 = freq;
        alex_write();
      }
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[1] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[2] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 10:
    case 11:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[3] = (uint32_t)floor(freq / 125.0e6 * (1 << 30) + 0.5);
      break;
    case 18:
    case 19:
      data = (frame[2] & 0x40) << 9 | frame[4] << 8 | frame[3];
      if(alex_data_4 != data)
      {
        alex_data_4 = data;
        alex_write();
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
      else
      {
        *tx_level = (data + 1) * 128 - 1;
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
      break;
    case 22:
    case 23:
      cw_reversed = (frame[2] >> 6) & 1;
      cw_speed = frame[3] & 63;
      cw_mode = (frame[3] >> 6) & 3;
      cw_weight = frame[4] & 127;
      cw_spacing = (frame[4] >> 7) & 1;
      break;
    case 30:
    case 31:
      cw_int_data = frame[1] & 1;
      cw_delay = frame[3];
      if(i2c_codec)
      {
        data = frame[2];
        *dac_level = (data + 1) * 256 - 1;
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
  int data_offset, header_offset, buffer_offset;
  uint32_t counter;
  int32_t value;
  uint16_t audio[512];
  uint8_t data0[4096];
  uint8_t data1[4096];
  uint8_t data2[4096];
  uint8_t data3[4096];
  uint8_t buffer[25][1032];
  struct iovec iovec[25][1];
  struct mmsghdr datagram[25];
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
    *(uint32_t *)(buffer[i] + 0) = 0x0601feef;
    iovec[i][0].iov_base = buffer[i];
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

  /* reset codec fifo */
  *codec_rst |= 2;
  *codec_rst &= ~2;

  /* reset rx fifo */
  *rx_rst |= 1;
  *rx_rst &= ~1;

  while(1)
  {
    if(!enable_thread) break;

    size = receivers * 6 + 2;
    n = 504 / size;
    m = 256 / n;

    if(*adc_cntr >= 1024 || *rx_cntr >= 8192)
    {
      /* reset codec fifo */
      *codec_rst |= 2;
      *codec_rst &= ~2;

      /* reset rx fifo */
      *rx_rst |= 1;
      *rx_rst &= ~1;
    }

    while(*rx_cntr < m * n * 16) usleep(1000);

    if(--rate_counter == 0)
    {
      for(i = 0; i < m * n * 2; ++i)
      {
        audio[i] = *adc_data;
      }
      rate_counter = 1 << rate;
      k = 0;
    }

    for(i = 0; i < m * n * 16; i += 8)
    {
      *(uint64_t *)(data0 + i) = *rx_data;
      *(uint64_t *)(data1 + i) = *rx_data;
      *(uint64_t *)(data2 + i) = *rx_data;
      *(uint64_t *)(data3 + i) = *rx_data;
    }

    data_offset = 0;
    for(i = 0; i < m; ++i)
    {
      *(uint32_t *)(buffer[i] + 4) = htonl(counter);

      memcpy(buffer[i] + 8, header + header_offset, 8);
      buffer[i][11] |= *gpio_in & 7;
      if(header_offset == 8)
      {
        value = xadc[153] >> 3;
        buffer[i][12] = (value >> 8) & 0xff;
        buffer[i][13] = value & 0xff;
        value = xadc[152] >> 3;
        buffer[i][14] = (value >> 8) & 0xff;
        buffer[i][15] = value & 0xff;
      }
      else if(header_offset == 16)
      {
        value = xadc[144] >> 3;
        buffer[i][12] = (value >> 8) & 0xff;
        buffer[i][13] = value & 0xff;
        value = xadc[145] >> 3;
        buffer[i][14] = (value >> 8) & 0xff;
        buffer[i][15] = value & 0xff;
      }
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;
      memset(buffer[i] + 16, 0, 504);

      buffer_offset = 16;
      for(j = 0; j < n; ++j)
      {
        memcpy(buffer[i] + buffer_offset, data0 + data_offset, 6);
        if(size > 8)
        {
          memcpy(buffer[i] + buffer_offset + 6, data1 + data_offset, 6);
        }
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        memcpy(buffer[i] + buffer_offset + size - 2, &audio[(k++) >> rate], 2);
        data_offset += 8;
        buffer_offset += size;
      }

      memcpy(buffer[i] + 520, header + header_offset, 8);
      buffer[i][523] |= *gpio_in & 7;
      if(header_offset == 8)
      {
        value = xadc[153] >> 3;
        buffer[i][524] = (value >> 8) & 0xff;
        buffer[i][525] = value & 0xff;
        value = xadc[152] >> 3;
        buffer[i][526] = (value >> 8) & 0xff;
        buffer[i][527] = value & 0xff;
      }
      else if(header_offset == 16)
      {
        value = xadc[144] >> 3;
        buffer[i][524] = (value >> 8) & 0xff;
        buffer[i][525] = value & 0xff;
        value = xadc[145] >> 3;
        buffer[i][526] = (value >> 8) & 0xff;
        buffer[i][527] = value & 0xff;
      }
      header_offset = header_offset >= 32 ? 0 : header_offset + 8;
      memset(buffer[i] + 528, 0, 504);

      buffer_offset = 528;
      for(j = 0; j < n; ++j)
      {
        memcpy(buffer[i] + buffer_offset, data0 + data_offset, 6);
        if(size > 8)
        {
          memcpy(buffer[i] + buffer_offset + 6, data1 + data_offset, 6);
        }
        if(size > 14)
        {
          memcpy(buffer[i] + buffer_offset + 12, data2 + data_offset, 6);
        }
        if(size > 20)
        {
          memcpy(buffer[i] + buffer_offset + 18, data3 + data_offset, 6);
        }
        memcpy(buffer[i] + buffer_offset + size - 2, &audio[(k++) >> rate], 2);
        data_offset += 8;
        buffer_offset += size;
      }

      ++counter;
    }

    sendmmsg(sock_ep2, datagram, m, 0);
  }

  active_thread = 0;

  return NULL;
}

void *handler_playback(void *arg)
{
  uint8_t buffer[2048];

  while(1)
  {
    while(jack_ringbuffer_read_space(playback_data) < 2048) usleep(1000);
    jack_ringbuffer_read(playback_data, buffer, 2048);
    write(1, buffer, 2048);
  }
  return NULL;
}
