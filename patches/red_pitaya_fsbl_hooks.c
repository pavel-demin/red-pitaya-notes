#define _GNU_SOURCE

#include <stdlib.h>
#include <string.h>

#include "xiicps.h"
#include "xemacps.h"

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

  Status = XIicPs_MasterSendPolled(&Iic, Buffer, 2, 0x50);
  if(Status != XST_SUCCESS) return XST_FAILURE;

  while(XIicPs_BusIsBusy(&Iic));

  Status = XIicPs_MasterRecvPolled(&Iic, Buffer, 1024, 0x50);
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

  return Status;
}
