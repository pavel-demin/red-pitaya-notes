// 19.4.2016 DC2PD : Add code to support outputs for bandpass and antenna switching via I2C chip.

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

#define I2C_SLAVE_FORCE 		   0x0706
#define I2C_SLAVE    			   0x0703    /* Change slave address            */
#define I2C_FUNCS    			   0x0705    /* Get the adapter functionality   */
#define I2C_RDWR    			   0x0707    /* Combined R/W transfer (one stop only)*/
 
#define penelop_ADDR 0x20   // PCA9555 switch to address 0
#define alex_ADDR 0x21      // PCA9555 switch to address 1 

uint32_t *rx_freq[4], *rx_rate, *tx_freq;
uint16_t *rx_cntr[4], *tx_cntr;
uint8_t *gpio_in, *gpio_out, *rx_rst, *tx_rst;
uint64_t *rx_data[4];
void *tx_data;

const uint32_t freq_min = 0;
const uint32_t freq_max = 61440000;

int receivers = 1;

int sock_ep2;
struct sockaddr_in addr_ep6;

int enable_thread = 0;
int active_thread = 0;

int vna = 0;

void process_ep2(uint8_t *frame);
void *handler_ep6(void *arg);

// variables to handle PCA9555 board 
int i2c_hdl, i2c_penelop , i2c_alex;

// Helper Functions to control 16 Bit I2C port

// read 16 bit from port
int PCA9555_read(uint16_t input){
   ssize_t bytes_written;
   ssize_t bytes_read;
   uint8_t write_buffer[1];
   uint8_t read_buffer[2];
   
   write_buffer[0] = 0x00;  // input register 0	
   // Write the register address onto the bus 
   bytes_written = write(i2c_hdl, write_buffer, 1);
   if(bytes_written < 0){
	return -1;
   } 
   // read 2 bytes from i2c	
   bytes_read = read(i2c_hdl, read_buffer, 2);
   if(bytes_read < 0){
	return -1;
   }
   input = (read_buffer[1] << 8) | read_buffer[0];
   return 0;	 		
}

// write 16 bit to port
int PCA9555_write(uint16_t output){
   ssize_t bytes_written;
   uint8_t write_buffer[3];

   write_buffer[0] = 0x02;   // output register
   write_buffer[1] = output;
   write_buffer[2] = output >> 8;
   // Write the register address and output to I2C 
   bytes_written = write(i2c_hdl, write_buffer, 3);
   if(bytes_written < 0){
	return -1;
   }
   return 0;
}

// write config register. Direction : 1 input ; 0 output 
int PCA9555_config(uint16_t config){
   ssize_t bytes_written;
   uint8_t write_buffer[3];

   write_buffer[0] = 0x06;   // config register
   write_buffer[1] = config ;
   write_buffer[2] = config >> 8;
   // Write the register address and config to I2C 
   bytes_written = write(i2c_hdl, write_buffer, 3);
   if(bytes_written < 0){
	return -1;
   }
   return 0;
}


/* main loop            */

int main(int argc, char *argv[])
{
  int fd, i;
  int status;
  ssize_t size;
  pthread_t thread;
  void *cfg, *sts;
  char *name = "/dev/mem";
  uint8_t buffer[1032];
  uint8_t reply[11] = {0xef, 0xfe, 2, 0, 0, 0, 0, 0, 0, 21, 0};
  struct ifreq hwaddr;
  struct sockaddr_in addr_ep2, addr_from;
  socklen_t size_from;
  int yes = 1;

  if((fd = open(name, O_RDWR)) < 0)
  {
    perror("open");
    return EXIT_FAILURE;
  }

  /* check if port extender on i2c port */
  i2c_penelop = 0;
  i2c_alex = 0;
  
  i2c_hdl = open("/dev/i2c-0", O_RDWR);    // open I2C
  if(i2c_hdl >= 0){
  	status = ioctl(i2c_hdl, I2C_SLAVE_FORCE, penelop_ADDR);   // select penelope outputs
    	if(status >= 0)
    	{
	  status = PCA9555_write(0x0000);          // resetset all pins
	  if(status == 0){
		 i2c_penelop = 1;	    	   // the board is present	
		 status = PCA9555_config(0x0000);  // set all 16-bit to output
		 if(status == 0) perror("i2c Penelope found ");  // inform user 
		}
	}
	status = ioctl(i2c_hdl, I2C_SLAVE, alex_ADDR);   // select alex outputs
    	if(status >= 0)
    	{
	  status = PCA9555_write(0x0000);   		// resetset all pins
          if(status == 0){			
		 i2c_alex = 1;				//The board is present		
                 status = PCA9555_config(0x0000);  	// set all 16-bit to output
		 if(status == 0) perror("i2c Alex found ");   // inform user
		}
	}
  } 

  sts = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);
  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40001000);
  rx_data[0] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40002000);
  rx_data[1] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40004000);
  rx_data[2] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40006000);
  rx_data[3] = mmap(NULL, 2*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40008000);
  tx_data = mmap(NULL, 16*sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40010000);

  *(uint32_t *)(tx_data + 8) = 165;

  rx_rst = ((uint8_t *)(cfg + 0));
  tx_rst = ((uint8_t *)(cfg + 1));
  gpio_out = ((uint8_t *)(cfg + 2));

  rx_rate = ((uint32_t *)(cfg + 4));

  rx_freq[0] = ((uint32_t *)(cfg + 8));
  rx_cntr[0] = ((uint16_t *)(sts + 12));

  rx_freq[1] = ((uint32_t *)(cfg + 12));
  rx_cntr[1] = ((uint16_t *)(sts + 14));

  rx_freq[2] = ((uint32_t *)(cfg + 16));
  rx_cntr[2] = ((uint16_t *)(sts + 16));

  rx_freq[3] = ((uint32_t *)(cfg + 20));
  rx_cntr[3] = ((uint16_t *)(sts + 18));

  tx_freq = ((uint32_t *)(cfg + 32));
  tx_cntr = ((uint16_t *)(sts + 20));

  gpio_in = ((uint8_t *)(sts + 22));

  /* set I/Q data for the VNA mode */
  *((uint64_t *)(cfg + 24)) = 2000000;
  *tx_rst &= ~2;

  /* set all GPIO pins to low */
  *gpio_out = 0;

  /* set default rx phase increment */
  *rx_freq[0] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[1] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[2] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  *rx_freq[3] = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);
  /* set default rx sample rate */
  *rx_rate = 1000;

  /* set default tx phase increment */
  *tx_freq = (uint32_t)floor(600000/125.0e6*(1<<30)+0.5);

  *tx_rst |= 1;
  *tx_rst &= ~1;

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
    size_from = sizeof(addr_from);
    size = recvfrom(sock_ep2, buffer, 1032, 0, (struct sockaddr *)&addr_from, &size_from);
    if(size < 0)
    {
      perror("recvfrom");
      return EXIT_FAILURE;
    }

    switch(*(uint32_t *)buffer)
    {
      case 0x0201feef:
        while(*tx_cntr > 16258) usleep(1000);
        if(*tx_cntr == 0) memset(tx_data, 0, 65032);
        if((*gpio_out & 1) | (*gpio_in & 1))
        {
          for(i = 0; i < 504; i += 8) memcpy(tx_data, buffer + 20 + i, 4);
          for(i = 0; i < 504; i += 8) memcpy(tx_data, buffer + 532 + i, 4);
        }
        else
        {
          memset(tx_data, 0, 504);
        }
        process_ep2(buffer + 11);
        process_ep2(buffer + 523);
        break;
      case 0x0002feef:
        reply[2] = 2 + active_thread;
        memset(buffer, 0, 60);
        memcpy(buffer, reply, 11);
        sendto(sock_ep2, buffer, 60, 0, (struct sockaddr *)&addr_from, size_from);
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
        addr_ep6.sin_addr.s_addr = addr_from.sin_addr.s_addr;
        addr_ep6.sin_port = addr_from.sin_port;
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

  close(sock_ep2);

  return EXIT_SUCCESS;
}

/* End Point 2 : Data from PC to HPSDR   */ 
void process_ep2(uint8_t *frame)  
{
  uint32_t freq;
  uint8_t pen_out;
  uint8_t antenna;
  uint8_t att;
  uint8_t tmp;
  int status;
  int change_p,change_a;
  uint8_t a1_out,a2_out;

  switch(frame[0])
  {
    case 0:
    case 1:  
      receivers = ((frame[4] >> 3) & 7) + 1;
      /* set PTT pin */
      if(frame[0] & 1) *gpio_out |= 1;
      else *gpio_out &= ~1;
      /* set preamp pin */
      if(frame[3] & 4) *gpio_out |= 2;
      else *gpio_out &= ~2;	

      /* set rx sample rate */
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

	if(i2c_penelop == 1){   	   // penelope I2C extension is attached
          change_p = 0;
	  if(pen_out != (frame[2] >> 1)){  // Penelope outputs changed
            change_p = 1;
 	    pen_out = frame[2] >> 1;       // output pins
	    }
	  if(antenna != ((frame[3] & 0x60) >> 5) | ((frame[4] & 0x03) << 2)){
	    change_p = 1;
            antenna = ((frame[3] & 0x60) >> 5) | ((frame[4] & 0x03) << 2);  // Rx Tx Antenna setting	
	    }
          if(att != (frame[3] & 0x03)){    // check if new setting
	    change_p = 1;
	    att = frame[3] & 0x03;           // new Attentuator setting
	    }		    

	  if(change_p = 1){		     // some bits have changed               
	    status = ioctl(i2c_hdl, I2C_SLAVE, penelop_ADDR);   // select penelope I2C outputs
	    status = PCA9555_write(pen_out | (att << 7) | (antenna << 9));   // set new data
	}
      }  	

      break;
    case 2:
    case 3:
      /* set tx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *tx_freq = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      if(!vna) break;
    case 4:
    case 5:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[0] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 6:
    case 7:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[1] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 8:
    case 9:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[2] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 10:
    case 11:
      /* set rx phase increment */
      freq = ntohl(*(uint32_t *)(frame + 1));
      if(freq < freq_min || freq > freq_max) break;
      *rx_freq[3] = (uint32_t)floor(freq/125.0e6*(1<<30)+0.5);
      break;
    case 18:
    case 19:
      /* set VNA mode */
      vna = frame[2] & 128;
      if(vna) *tx_rst |= 2;
      else *tx_rst &= ~2;

      if(i2c_alex == 1){   // alex I2C extension is attached	
	change_a = 0;
	if(a1_out != frame[3]){  // Alex outputs changed
            change_a = 1;
 	    a1_out = frame[3];   // Alex output pins1
	    }
	if(a2_out != frame[4]){  // Alex outputs changed
            change_a = 1;
 	    a2_out = frame[4];   // Alex output pins2
	    }
	if(change_a = 1){	 // some Alex bits have changed               
	    status = ioctl(i2c_hdl, I2C_SLAVE, alex_ADDR);   // select alex I2C outputs
	    status = PCA9555_write(a1_out | (a2_out << 8));  // set new data
	}
	
      }	
      break;
  }
}

void *handler_ep6(void *arg)
{
  int i, j, n, m, size;
  int data_offset, header_offset, buffer_offset;
  uint32_t counter;
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

  *rx_rst |= 1;
  *rx_rst &= ~1;

  while(1)
  {
    if(!enable_thread) break;

    size = receivers * 6 + 2;
    n = 504 / size;
    m = 256 / n;

    if(*rx_cntr[0] >= 2048)
    {
      *rx_rst |= 1;
      *rx_rst &= ~1;
    }

    while(*rx_cntr[0] < m * n * 4) usleep(1000);

    for(i = 0; i < m * n * 16; i += 8)
    {
       *(uint64_t *)(data0 + i) = *rx_data[0];
       *(uint64_t *)(data1 + i) = *rx_data[1];
       *(uint64_t *)(data2 + i) = *rx_data[2];
       *(uint64_t *)(data3 + i) = *rx_data[3];
    }

    data_offset = 0;
    for(i = 0; i < m; ++i)
    {
      *(uint32_t *)(buffer[i] + 4) = htonl(counter);

      memcpy(buffer[i] + 8, header + header_offset, 8);
      buffer[i][11] |= *gpio_in & 7;
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
        data_offset += 8;
        buffer_offset += size;
      }

      memcpy(buffer[i] + 520, header + header_offset, 8);
      buffer[i][523] |= *gpio_in & 7;
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
