
#include <string.h>
#include <stdio.h>

#include <windows.h>
#include <winsock.h>
#pragma comment (lib, "wsock32.lib")

//---------------------------------------------------------------------------

#define EXTIO_API __declspec(dllexport) __stdcall

#define FREQ_PORT 1001
#define DATA_PORT 1002
#define MCAST_ADDR "239.0.0.39"

int gFreqSock, gDataSock;
struct sockaddr_in gFreqAddr, gDataAddr;

HANDLE gTriggerEvent = NULL;
HANDLE gTimerQueue = NULL;
HANDLE gTimer = NULL;

char gBuffer[4096];
int gOffset = 0;

long gFreq = 550000;

bool gInitHW = false;

bool gExitThread = false;
bool gThreadRunning = false;

//---------------------------------------------------------------------------

void (*ExtIOCallback)(int, int, float, void *) = NULL;

//---------------------------------------------------------------------------

DWORD WINAPI GeneratorThreadProc(__in  LPVOID lpParameter)
{
  unsigned long size = 0;

  while(!gExitThread)
  {
    SleepEx(1, FALSE );
    if(gExitThread) break;

    ioctlsocket(gDataSock, FIONREAD, &size);

    while(size >= 1024)
    {
      recvfrom(gDataSock, gBuffer + gOffset, 1024, 0, NULL, 0);

      gOffset += 1024;
      if(gOffset == 4096)
      {
        gOffset = 0;
        (*ExtIOCallback)(512, 0, 0, gBuffer);
      }

      ioctlsocket(gDataSock, FIONREAD, &size);
    }
  }
  gExitThread = false;
  gThreadRunning = false;
  return 0;
}

//---------------------------------------------------------------------------

static void stopThread()
{
  if(gThreadRunning)
  {
    gExitThread = true;
    while(gThreadRunning)
    {
      SleepEx(10, FALSE);
    }
  }
}

//---------------------------------------------------------------------------

static void startThread()
{
  gExitThread = false;
  gThreadRunning = true;
  CreateThread(NULL, (SIZE_T)(64 * 1024), GeneratorThreadProc, NULL, 0, NULL);
}

//---------------------------------------------------------------------------

extern "C"
bool EXTIO_API InitHW(char *name, char *model, int &type)
{
  type = 6;

  strcpy(name, "Red Pitaya SDR");
  strcpy(model, "");

  if(!gInitHW)
  {
    gFreq = 550000;
    gInitHW = true;
  }

  return gInitHW;
}

//---------------------------------------------------------------------------

extern "C"
bool EXTIO_API OpenHW()
{
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 0), &wsaData);

  gFreqSock = socket(AF_INET, SOCK_DGRAM, 0);

  memset(&gDataAddr, 0, sizeof(gDataAddr));
  gFreqAddr.sin_family = AF_INET;
  gFreqAddr.sin_addr.s_addr = inet_addr(MCAST_ADDR);
  gFreqAddr.sin_port = htons(FREQ_PORT);

  return gInitHW;
}

//---------------------------------------------------------------------------

extern "C"
int  EXTIO_API StartHW(long LOfreq)
{
  struct ip_mreq mreq;
  WSADATA wsaData;

  if(!gInitHW) return 0;

  WSAStartup(MAKEWORD(2, 0), &wsaData);

  gDataSock = socket(AF_INET,SOCK_DGRAM,0);

  memset(&gDataAddr, 0, sizeof(gDataAddr));
  gDataAddr.sin_family = AF_INET;
  gDataAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  gDataAddr.sin_port = htons(DATA_PORT);

  bind(gDataSock, (struct sockaddr *) &gDataAddr, sizeof(gDataAddr));

  mreq.imr_multiaddr.s_addr = inet_addr(MCAST_ADDR);
  mreq.imr_interface.s_addr = htonl(INADDR_ANY);
  setsockopt(gDataSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &mreq, sizeof(mreq));

  stopThread();
  startThread();

  return 512;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API StopHW()
{
  stopThread();
  closesocket(gDataSock);
  WSACleanup();
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API CloseHW()
{
  closesocket(gFreqSock);
  WSACleanup();
  gInitHW = false;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API SetCallback(void (*callback)(int, int, float, void *))
{
  ExtIOCallback = callback;
  (*ExtIOCallback)(-1, 101, 0, NULL);
}

//---------------------------------------------------------------------------

extern "C"
int  EXTIO_API SetHWLO(long LOfreq)
{
  gFreq = LOfreq;
  sendto(gFreqSock, (char *) &gFreq, 4, 0, (struct sockaddr *) &gFreqAddr, sizeof(gFreqAddr));
  return 0;
}

//---------------------------------------------------------------------------

extern "C"
long EXTIO_API GetHWLO()
{
  return gFreq;
}

//---------------------------------------------------------------------------

extern "C"
long EXTIO_API GetHWSR()
{
  return 100000;
}

//---------------------------------------------------------------------------

extern "C"
int  EXTIO_API GetStatus()
{
  return 0;
}
