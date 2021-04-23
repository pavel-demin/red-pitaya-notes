#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <alsa/asoundlib.h>
#include <fftw3.h>

#include "comm.h"

#define I2C_SLAVE       0x0703 /* Use this slave address */
#define I2C_SLAVE_FORCE 0x0706 /* Use this slave address, even if it
                                  is already in use by a driver! */

#define ADDR_CODEC 0x1A /* WM8731 or TLV320AIC23B address 0 */

volatile float *rx_data, *sp_data, *tx_data;
volatile uint16_t *rx_cntr, *tx_cntr, *dac_cntr, *adc_cntr;
volatile uint8_t *rx_rst, *sp_rst, *tx_rst, *codec_rst;
volatile int32_t *dac_data, *adc_data;
volatile int32_t *sp_win;
volatile int32_t *xadc;

int i2c_codec = 0;

ssize_t i2c_write8(int fd, uint8_t addr, uint8_t data)
{
  uint8_t buffer[2];
  buffer[0] = addr;
  buffer[1] = data;
  return write(fd, buffer, 2);
}

void *rx_data_handler(void *arg);
void *tx_data_handler(void *arg);

int main()
{
  int fd, i2c_fd, uart_fd, running, filter, mode, ptt, mux, i, size;
  FILE *wisdom_file;
  pthread_t thread;
  volatile void *cfg, *sts;
  volatile uint32_t *rx_phase, *sp_phase, *tx_phase, *dac_phase, *alex, *tx_mux, *dac_mux;
  volatile int32_t *tx_ramp, *dac_ramp;
  volatile uint16_t *tx_size, *dac_size, *sp_rate, *sp_total, *sp_scale, *sp_cntr;
  volatile uint16_t *tx_level, *dac_level;
  volatile uint8_t *gpio_in, *gpio_out;
  volatile float *sp_corr;
  float scale, value, ramp[1024], a[4] = {0.35875, 0.48829, 0.14128, 0.01168};
  unsigned long num = 0;
  struct termios tty;
  uint8_t buffer[6];
  uint8_t code;
  uint32_t data;
  uint8_t crc8;
  uint8_t volume;
  int32_t rx_freq, sp_freq, tx_freq, shift;
  long min, max;
  snd_mixer_t *handle;
  snd_mixer_selem_id_t *sid;
  snd_mixer_elem_t *playback, *capture;
  double cutoff[5][10][2] = {
    {{-613, -587}, {-625, -575}, {-650, -550}, {-725, -475}, {-800, -400}, {-850, -350}, {-900, -300}, {-975, -225}, {-1000, -200}, {-1100, -100}},
    {{587, 613}, {575, 625}, {550, 650}, {475, 725}, {400, 800}, {350, 850}, {300, 900}, {225, 975}, {200, 1000}, {100, 1100}},
    {{-1150, -150}, {-1950, -150}, {-2250, -150}, {-2550, -150}, {-2850, -150}, {-3050, -150}, {-3450, -150}, {-3950, -150}, {-4550, -150}, {-5150, -150}},
    {{150, 1150}, {150, 1950}, {150, 2250}, {150, 2550}, {150, 2850}, {150, 3050}, {150, 3450}, {150, 3950}, {150, 4550}, {150, 5150}},
    {{-2500, 2500}, {-3000, 3000}, {-3500, 3500}, {-4000, 4000}, {-4500, 4500}, {-5000, 5000}, {-6000, 6000}, {-8000, 8000}, {-9000, 9000}, {-10000, 10000}}};

  if((fd = open("/dev/mem", O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  if((i2c_fd = open("/dev/i2c-0", O_RDWR)) >= 0)
  {
    if(ioctl(i2c_fd, I2C_SLAVE_FORCE, ADDR_CODEC) >= 0)
    {
      /* reset */
      if(i2c_write8(i2c_fd, 0x1e, 0x00) > 0)
      {
        i2c_codec = 1;
        /* set power down register */
        i2c_write8(i2c_fd, 0x0c, 0x51);
        /* reset activate register */
        i2c_write8(i2c_fd, 0x12, 0x00);
        /* set volume to -30 dB */
        i2c_write8(i2c_fd, 0x04, 0x5b);
        i2c_write8(i2c_fd, 0x06, 0x5b);
        /* set analog audio path register */
        i2c_write8(i2c_fd, 0x08, 0x15);
        /* set digital audio path register */
        i2c_write8(i2c_fd, 0x0a, 0x00);
        /* set format register */
        i2c_write8(i2c_fd, 0x0e, 0x42);
        /* set activate register */
        i2c_write8(i2c_fd, 0x12, 0x01);
        /* set power down register */
        i2c_write8(i2c_fd, 0x0c, 0x41);
      }
    }
  }

  if((uart_fd = open("/dev/ttyPS1", O_RDWR|O_NOCTTY|O_NDELAY)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  tcgetattr(uart_fd, &tty);
  cfsetspeed(&tty, (speed_t)B115200);
  cfmakeraw(&tty);
  tty.c_cflag &= ~(CSTOPB | CRTSCTS);
  tty.c_cflag |= CLOCAL | CREAD;
  tcflush(uart_fd, TCIFLUSH);
  tcsetattr(uart_fd, TCSANOW, &tty);

  if(!i2c_codec)
  {
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, "default");
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Speaker");
    playback = snd_mixer_find_selem(handle, sid);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Mic");
    capture = snd_mixer_find_selem(handle, sid);
  }

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  alex = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rx_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40003000);
  tx_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40005000);
  tx_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
  tx_mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40007000);
  sp_win = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
  sp_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40009000);
  dac_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000A000);
  dac_ramp = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000B000);
  dac_mux = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000C000);
  adc_data = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x4000D000);
  xadc = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40020000);

  rx_rst = ((uint8_t *)(cfg + 0));
  sp_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  codec_rst = ((uint8_t *)(cfg + 2));
  gpio_out = ((uint8_t *)(cfg + 3));

  rx_phase = ((uint32_t *)(cfg + 4));

  sp_rate = ((uint16_t *)(cfg + 12));
  sp_phase = ((uint32_t *)(cfg + 16));
  sp_total = ((uint16_t *)(cfg + 20));
  sp_scale = ((uint16_t *)(cfg + 22));
  sp_corr = ((float *)(cfg + 24));

  tx_phase = ((uint32_t *)(cfg + 28));
  tx_size = ((uint16_t *)(cfg + 32));
  tx_level = ((uint16_t *)(cfg + 34));

  dac_phase = ((uint32_t *)(cfg + 36));
  dac_size = ((uint16_t *)(cfg + 40));
  dac_level = ((uint16_t *)(cfg + 42));

  rx_cntr = ((uint16_t *)(sts + 12));
  sp_cntr = ((uint16_t *)(sts + 18));
  tx_cntr = ((uint16_t *)(sts + 20));
  dac_cntr = ((uint16_t *)(sts + 22));
  adc_cntr = ((uint16_t *)(sts + 24));
  gpio_in = ((uint8_t *)(sts + 26));

  /* set PTT pin to low */
  *gpio_out = 0;

  if((wisdom_file = fopen("wdsp-fftw-wisdom.txt", "r")))
  {
    fftw_import_wisdom_from_file(wisdom_file);
    fclose(wisdom_file);
  }

  OpenChannel(0, 256, 2048, 48000, 48000, 48000, 0, 0, 0.010, 0.025, 0.000, 0.010, 0);
  OpenChannel(1, 256, 2048, 48000, 48000, 48000, 1, 0, 0.010, 0.025, 0.000, 0.010, 0);

  if((wisdom_file = fopen("wdsp-fftw-wisdom.txt", "w")))
  {
    fftw_export_wisdom_to_file(wisdom_file);
    fclose(wisdom_file);
  }

  SetRXAShiftRun(0, 0);
  SetRXAPanelGain1(0, 1.0);
  SetRXAAGCMode(0, 3);
  RXASetNC(0, 2048);
  RXASetMP(0, 1);

  SetTXACompressorRun(1, 1);
  SetPSRunCal(1, 0);
  TXASetNC(1, 2048);
  TXASetMP(1, 1);

  mode = 4;
  shift = 0;
  filter = 5;
  rx_freq = 621000;
  sp_freq = 621000;
  tx_freq = 621000;

  *rx_phase = (uint32_t)floor((rx_freq + shift) / 122.88e6 * (1<<30) + 0.5);
  *sp_phase = (uint32_t)floor(sp_freq / 122.88e6 * (1<<30) + 0.5);
  *tx_phase = (uint32_t)floor(tx_freq / 122.88e6 * (1<<30) + 0.5);

  SetRXAMode(0, RXA_AM);
  SetTXAMode(1, TXA_AM);

  RXASetPassband(0, cutoff[mode][filter][0], cutoff[mode][filter][1]);
  SetTXABandpassFreqs(1, cutoff[mode][filter][0], cutoff[mode][filter][1]);

  *sp_rate = 125;
  *sp_total = 32767;
  *sp_scale = 1024;
  *sp_corr = 8.68589;

  /* set sp window */
  size = 512;
  for(i = 0; i < size; ++i)
  {
    value = a[0] - a[1] * cos(2.0 * M_PI * i / (size - 1)) + a[2] * cos(4.0 * M_PI * i / (size - 1)) - a[3] * cos(6.0 * M_PI * i / (size - 1));
    sp_win[i] = (int32_t)floor(value * 8388607 + 0.5);
  }

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
    tx_ramp[i] = (uint32_t)floor(ramp[i] * scale + 0.5);
  }
  *tx_size = size;

  /* set default tx level */
  *tx_level = 0;

  /* set default tx mux channel */
  *(tx_mux + 16) = 0;
  *tx_mux = 2;

  *rx_rst |= 1;
  *rx_rst &= ~1;

  *sp_rst &= ~2;
  *sp_rst |= 4;
  *sp_rst &= ~4;
  *sp_rst |= 2;

  *tx_rst |= 1;
  *tx_rst &= ~1;

  if(i2c_codec)
  {
    /* reset codec fifo buffers */
    *codec_rst |= 3;
    *codec_rst &= ~3;
    /* enable I2S interface */
    *codec_rst &= ~4;

    /* set default dac phase increment */
    *dac_phase = (uint32_t)floor(600 / 48.0e3 * (1 << 30) + 0.5);

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
    *dac_level = 32766;

    /* set default dac mux channel */
    *(dac_mux + 16) = 0;
    *dac_mux = 2;
  }
  else
  {
    /* enable ALEX interface */
    *codec_rst |= 4;
  }

  SetChannelState(0, 1, 0);
  SetChannelState(1, 1, 0);

  if(pthread_create(&thread, NULL, rx_data_handler, NULL) < 0)
  {
    perror("pthread_create");
    return EXIT_FAILURE;
  }
  pthread_detach(thread);

  if(pthread_create(&thread, NULL, tx_data_handler, NULL) < 0)
  {
    perror("pthread_create");
    return EXIT_FAILURE;
  }
  pthread_detach(thread);

  running = 1;
  while(running)
  {
    usleep(50000);
    /* update RX meter */
    *(uint8_t *)(buffer + 0) = 85;
    *(uint32_t *)(buffer + 1) = (uint32_t)floor(-10.0 * GetRXAMeter(0, 1) + 0.5);
    *(uint8_t *)(buffer + 5) = 160;
    write(uart_fd, buffer, 6);
    while(1)
    {
      ioctl(uart_fd, FIONREAD, &num);
      if(num < 6) break;
      read(uart_fd, buffer, 6);
      code = *(uint8_t *)(buffer + 0);
      data = *(uint32_t *)(buffer + 1);
      crc8 = *(uint8_t *)(buffer + 5);
      switch(code)
      {
        case 1:
          if(data > 62000000) continue;
          rx_freq = data;
          *rx_phase = (uint32_t)floor((rx_freq + shift) / 122.88e6 * (1<<30) + 0.5);
          break;
        case 2:
          if(data > 62000000) continue;
          tx_freq = data;
          *tx_phase = (uint32_t)floor(tx_freq / 122.88e6 * (1<<30) + 0.5);
          break;
        case 3:
          if(data > 9) continue;
          if(i2c_codec)
          {
            volume = 48 + data * 7;
            i2c_write8(i2c_fd, 0x04, volume);
            i2c_write8(i2c_fd, 0x06, volume);
          }
          else
          {
            snd_mixer_selem_get_playback_volume_range(playback, &min, &max);
            snd_mixer_selem_set_playback_volume_all(playback, data * max / 9);
          }
          break;
        case 4:
          if(data > 9) continue;
          if(i2c_codec)
          {
          }
          else
          {
            snd_mixer_selem_get_capture_volume_range(capture, &min, &max);
            snd_mixer_selem_set_capture_volume_all(capture, data * max / 9);
          }
          break;
        case 5:
          if(data > 9) continue;
          filter = data;
          RXASetPassband(0, cutoff[mode][filter][0], cutoff[mode][filter][1]);
          SetTXABandpassFreqs(1, cutoff[mode][filter][0], cutoff[mode][filter][1]);
          break;
        case 6:
          if(data > 4) continue;
          mode = data;
          switch(mode)
          {
            case 0:
              shift = 600;
              SetRXAMode(0, RXA_CWL);
              SetTXAMode(1, TXA_CWL);
              break;
            case 1:
              shift = -600;
              SetRXAMode(0, RXA_CWU);
              SetTXAMode(1, TXA_CWU);
              break;
            case 2:
              shift = 0;
              SetRXAMode(0, RXA_LSB);
              SetTXAMode(1, TXA_LSB);
              break;
            case 3:
              shift = 0;
              SetRXAMode(0, RXA_USB);
              SetTXAMode(1, TXA_USB);
              break;
            case 4:
              shift = 0;
              SetRXAMode(0, RXA_AM);
              SetTXAMode(1, TXA_AM);
              break;
          }
          *rx_phase = (uint32_t)floor((rx_freq + shift) / 122.88e6 * (1<<30) + 0.5);
          RXASetPassband(0, cutoff[mode][filter][0], cutoff[mode][filter][1]);
          SetTXABandpassFreqs(1, cutoff[mode][filter][0], cutoff[mode][filter][1]);
          break;
        case 8:
          if(data > 1) continue;
          ptt = data;
          *gpio_out = data;
          *tx_level = data ? 32110 : 0;
          break;
        case 9:
          running = 0;
          break;
      }
      data = ptt & (mode < 2);
      if(mux != data)
      {
        mux = data;
        *(tx_mux + 16) = data;
        *tx_mux = 2;
        if(i2c_codec)
        {
          *(dac_mux + 16) = data;
          *dac_mux = 2;
        }
      }

    }
  }

  /* set PTT pin to low */
  *gpio_out = 0;

  return EXIT_SUCCESS;
}

void *rx_data_handler(void *arg)
{
  int32_t i, error, value;
  double buffer0[512], buffer1[512];
  float buffer2[512];

  while(1)
  {
    if(*rx_cntr >= 1024)
    {
      *rx_rst |= 1;
      *rx_rst &= ~1;
    }

    while(*rx_cntr < 512) usleep(1000);

    for(i = 0; i < 512; ++i) buffer0[i] = *rx_data;

    fexchange0(0, buffer0, buffer1, &error);

    if(i2c_codec)
    {
      while(*dac_cntr > 256) usleep(1000);
      if(*dac_cntr == 0) for(i = 0; i < 256; ++i) *dac_data = 0;
      for(i = 0; i < 512; i += 2)
      {
        value = (int32_t)floor(buffer1[i] * 32766.0 + 0.5) << 16;
        value |= (int32_t)floor(buffer1[i + 1] * 32766.0 + 0.5) & 0xffff;
        *dac_data = value;
      }
    }
    else
    {
      for(i = 0; i < 512; ++i) buffer2[i] = buffer1[i];
      fwrite(buffer2, 4, 512, stdout);
      fflush(stdout);
    }
  }

  return NULL;
}

void *tx_data_handler(void *arg)
{
  int32_t i, error, value;
  float buffer0[512];
  double buffer1[512], buffer2[512];

  while(1)
  {
    while(*tx_cntr > 512) usleep(1000);

    if(*tx_cntr == 0)
    {
      for(i = 0; i < 512; ++i) *tx_data = 0.0;
    }

    if(i2c_codec)
    {
      if(*adc_cntr >= 512)
      {
        *codec_rst |= 2;
        *codec_rst &= ~2;
      }
      while(*adc_cntr < 256) usleep(1000);
      for(i = 0; i < 512; i += 2)
      {
        value = *adc_data;
        buffer1[i] = (double)(value >> 16) / 32767.0;
        buffer1[i + 1] = (double)(value & 0xffff) / 32767.0;
      }
    }
    else
    {
      fread(buffer0, 4, 512, stdin);
      for(i = 0; i < 512; ++i) buffer1[i] = buffer0[i];
    }

    fexchange0(1, buffer1, buffer2, &error);

    for(i = 0; i < 512; ++i) *tx_data = buffer2[i];
  }

  return NULL;
}
