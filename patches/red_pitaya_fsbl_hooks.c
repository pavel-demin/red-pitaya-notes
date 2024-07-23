#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include "xiicps.h"
#include "xemacps.h"

const u8 ADDR_EEPROM = 0x50;
const u8 ADDR_SI5351 = 0x60;

const u32 XREF_FREQ = 27000000;

static u32 si5351_start(XIicPs* pIic);
static u32 si5351_setfreq(XIicPs* pIic, unsigned long freq);

u32 SetMacAddress()
{
  XIicPs Iic;
  XIicPs_Config *IicConfig;
  XEmacPs Emac;
  XEmacPs_Config *EmacConfig;
  u32 Status, i;
  u8 Buffer[1024];
  char *Pointer;

  Buffer[0] = 0x18;
  Buffer[1] = 0;

  IicConfig = XIicPs_LookupConfig(XPAR_PS7_I2C_0_DEVICE_ID);
  if(IicConfig == NULL) return XST_FAILURE;

  Status = XIicPs_CfgInitialize(&Iic, IicConfig, IicConfig->BaseAddress);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  Status = XIicPs_SetSClk(&Iic, 100000);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  Status = XIicPs_MasterSendPolled(&Iic, Buffer, 2, ADDR_EEPROM);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  while(XIicPs_BusIsBusy(&Iic));

  Status = XIicPs_MasterRecvPolled(&Iic, Buffer, 1024, ADDR_EEPROM);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  Pointer = memmem(Buffer, 1024, "ethaddr=", 8);
  if(Pointer == NULL) return XST_FAILURE;

  Pointer += 7;
  for(i = 0; i < 6; ++i)
  {
    Buffer[i] = strtol(Pointer + 1, &Pointer, 16);
  }

  EmacConfig = XEmacPs_LookupConfig(XPAR_PS7_ETHERNET_0_DEVICE_ID);
  if(EmacConfig == NULL) return XST_FAILURE;

  Status = XEmacPs_CfgInitialize(&Emac, EmacConfig, EmacConfig->BaseAddress);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  Status = XEmacPs_SetMacAddress(&Emac, Buffer, 1);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  /* Set Si5351 to 122.88MHz */
  Status = si5351_start(&Iic);
  if (Status != XST_SUCCESS) return XST_FAILURE;

  Status = si5351_setfreq(&Iic, 125000000);
  if (Status != XST_SUCCESS) return XST_FAILURE;

  return Status;
}

/* Si5351 simple driver*/
// Set of Si5351A register addresses
#define CLK_ENABLE_CONTROL 3
#define PLLX_SRC 15
#define CLK0_CONTROL 16
#define CLK1_CONTROL 17
#define CLK2_CONTROL 18
#define SYNTH_PLL_A 26
#define SYNTH_PLL_B 34
#define SYNTH_MS_0 42
#define SYNTH_MS_1 50
#define SYNTH_MS_2 58
#define PLL_RESET 177
#define XTAL_LOAD_CAP 183

#define si5351_write(reg_addr, reg_value)                                        \
  do                                                                             \
  {                                                                              \
    u8 data[2] = {(u8)reg_addr, (u8)reg_value};                                  \
    u32 Status = XIicPs_MasterSendPolled(pIic, data, sizeof(data), ADDR_SI5351); \
    if (Status != XST_SUCCESS)                                                   \
      return XST_FAILURE;                                                        \
  } while (0)

// Set PLLs (VCOs) to internal clock rate of 900 MHz
// Equation fVCO = fXTAL * (a+b/c) (=> AN619 p. 3
u32 si5351_start(XIicPs *pIic)
{
  unsigned long a, b, c;
  unsigned long p1, p2, p3;

  // Init clock chip
  si5351_write(XTAL_LOAD_CAP, 0xD2);      // Set crystal load capacitor to 10pF (default),
                                          // for bits 5:0 see also AN619 p. 60
  si5351_write(CLK_ENABLE_CONTROL, 0xFE); // Enable CLK 0 only
  si5351_write(CLK0_CONTROL, 0x0F);       // Set PLLA to CLK0, 8 mA output
  si5351_write(CLK1_CONTROL, 0x2F);       // Set PLLB to CLK1, 8 mA output
  si5351_write(CLK2_CONTROL, 0x2F);       // Set PLLB to CLK2, 8 mA output
  si5351_write(PLL_RESET, 0xA0);          // Reset PLLA and PLLB

  // Set VCOs of PLLA and PLLB to 900 MHz
  a = 36;      // Division factor 900/25 MHz
  b = 0;       // Numerator, sets b/c=0
  c = 1048575; // Max. resolution, but irrelevant in this case (b=0)

  // Formula for splitting up the numbers to register data, see AN619
  p1 = 128 * a + (unsigned long)(128 * b / c) - 512;
  p2 = 128 * b - c * (unsigned long)(128 * b / c);
  p3 = c;

  // Write data to registers PLLA and PLLB so that both VCOs are set to 900MHz intermal freq
  si5351_write(SYNTH_PLL_A, 0xFF);
  si5351_write(SYNTH_PLL_A + 1, 0xFF);
  si5351_write(SYNTH_PLL_A + 2, (p1 & 0x00030000) >> 16);
  si5351_write(SYNTH_PLL_A + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_A + 4, (p1 & 0x000000FF));
  si5351_write(SYNTH_PLL_A + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(SYNTH_PLL_A + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_A + 7, (p2 & 0x000000FF));

  si5351_write(SYNTH_PLL_B, 0xFF);
  si5351_write(SYNTH_PLL_B + 1, 0xFF);
  si5351_write(SYNTH_PLL_B + 2, (p1 & 0x00030000) >> 16);
  si5351_write(SYNTH_PLL_B + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_B + 4, (p1 & 0x000000FF));
  si5351_write(SYNTH_PLL_B + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(SYNTH_PLL_B + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(SYNTH_PLL_B + 7, (p2 & 0x000000FF));

  return XST_SUCCESS;
}

u32 si5351_setfreq(XIicPs *pIic, unsigned long freq)
{
  int synth = SYNTH_MS_0;
  unsigned long a, b, c = 1048575;
  unsigned long f_xtal = XREF_FREQ;
  double fdiv = (double)(f_xtal * 36) / freq; // division factor fvco/freq (will be integer part of a+b/c)
  double rm;                                  // remainder
  unsigned long p1, p2, p3;

  a = (unsigned long)fdiv;
  rm = fdiv - a; //(equiv. b/c)
  b = rm * c;
  p1 = 128 * a + (unsigned long)(128 * b / c) - 512;
  p2 = 128 * b - c * (unsigned long)(128 * b / c);
  p3 = c;

  // Write data to multisynth registers of synth n
  si5351_write(synth, 0xFF);     // 1048757 MSB
  si5351_write(synth + 1, 0xFF); // 1048757 LSB
  si5351_write(synth + 2, (p1 & 0x00030000) >> 16);
  si5351_write(synth + 3, (p1 & 0x0000FF00) >> 8);
  si5351_write(synth + 4, (p1 & 0x000000FF));
  si5351_write(synth + 5, 0xF0 | ((p2 & 0x000F0000) >> 16));
  si5351_write(synth + 6, (p2 & 0x0000FF00) >> 8);
  si5351_write(synth + 7, (p2 & 0x000000FF));

  return XST_SUCCESS;
}
