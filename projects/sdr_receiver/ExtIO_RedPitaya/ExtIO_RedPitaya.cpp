
#include <string.h>
#include <math.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>

#include "GUI.h"

using namespace System;
using namespace System::Runtime::InteropServices;

using namespace ExtIO_RedPitaya;

//---------------------------------------------------------------------------

#define EXTIO_API __declspec(dllexport) __stdcall

#define LO_MIN   100000
#define LO_MAX 50000000

SOCKET gSock;

char gBuffer[4096];
int gOffset = 0;

long gFreq = 600000;

bool gInitHW = false;

bool gExitThread = false;
bool gThreadRunning = false;

//---------------------------------------------------------------------------

ref class ManagedGlobals
{
  public:
    static GUI ^gGUI = nullptr;
};

//---------------------------------------------------------------------------

void (*ExtIOCallback)(int, int, float, void *) = 0;

//---------------------------------------------------------------------------

DWORD WINAPI GeneratorThreadProc(__in LPVOID lpParameter)
{
  unsigned long size = 0;

  while(!gExitThread)
  {
    SleepEx(1, FALSE );
    if(gExitThread) break;

    ioctlsocket(gSock, FIONREAD, &size);

    while(size >= 1024)
    {
      recv(gSock, gBuffer + gOffset, 1024, 0);

      gOffset += 1024;
      if(gOffset == 4096)
      {
        gOffset = 0;
        if(ExtIOCallback) (*ExtIOCallback)(512, 0, 0.0, gBuffer);
      }

      ioctlsocket(gSock, FIONREAD, &size);
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

extern "C" int EXTIO_API SetHWLO(long LOfreq);

//---------------------------------------------------------------------------

extern "C"
bool EXTIO_API InitHW(char *name, char *model, int &type)
{
  type = 6;

  strcpy(name, "Red Pitaya SDR");
  strcpy(model, "");

  if(!gInitHW)
  {
    ManagedGlobals::gGUI = gcnew GUI;
    gFreq = 600000;
    gInitHW = true;
  }

  return gInitHW;
}

//---------------------------------------------------------------------------

extern "C"
bool EXTIO_API OpenHW()
{
  return gInitHW;
}

//---------------------------------------------------------------------------

extern "C"
int EXTIO_API StartHW(long LOfreq)
{
  WSADATA wsaData;
  struct sockaddr_in addr;

  if(!gInitHW) return 0;

  WSAStartup(MAKEWORD(2, 2), &wsaData);

  gSock = socket(AF_INET, SOCK_STREAM, 0);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr((const char*)Marshal::StringToHGlobalAnsi(ManagedGlobals::gGUI->host->Text).ToPointer());
  addr.sin_port = htons((u_short)ManagedGlobals::gGUI->port->Value);

  connect(gSock, (struct sockaddr *)&addr, sizeof(addr));

  stopThread();
  SetHWLO(LOfreq);
  startThread();

  return 512;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API StopHW()
{
  stopThread();

  closesocket(gSock);

  WSACleanup();
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API CloseHW()
{
  gInitHW = false;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API SetCallback(void (*callback)(int, int, float, void *))
{
  ExtIOCallback = callback;
}

//---------------------------------------------------------------------------

extern "C"
int EXTIO_API SetHWLO(long LOfreq)
{
  long rc = 0;

  gFreq = LOfreq;

  // check limits
  if(gFreq < LO_MIN)
  {
    gFreq = LO_MIN;
    rc = -LO_MIN;
  }
  else if(gFreq > LO_MAX)
  {
    gFreq = LO_MAX;
    rc = LO_MAX;
  }

  gFreq = (long)floor(floor(gFreq/125.0e6*(1<<30)+0.5)*125e6/(1<<30)+0.5);

  if(gFreq != LOfreq && ExtIOCallback) (*ExtIOCallback)(-1, 101, 0.0, 0);

  send(gSock, (char *)&gFreq, 4, 0);

  return rc;
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
int EXTIO_API GetStatus()
{
  return 0;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API ShowGUI()
{
  if(ManagedGlobals::gGUI) ManagedGlobals::gGUI->ShowDialog();
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API HideGUI()
{
  if(ManagedGlobals::gGUI) ManagedGlobals::gGUI->Hide();
}