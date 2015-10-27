
#include <string.h>
#include <math.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <windows.h>

#include "GUI.h"

using namespace System;
using namespace System::Runtime::InteropServices;
using namespace Microsoft::Win32;

using namespace ExtIO_RedPitaya_TRX;

//---------------------------------------------------------------------------

#define EXTIO_API __declspec(dllexport) __stdcall

#define TCP_PORT 1001
#define TCP_ADDR "192.168.1.100"

SOCKET gSock[2] = {-1, -1};

char gBuffer[4096];

UInt32 gRate = 100000;
Int32 gCorr = 0;

UInt32 gFreq = 600000;
UInt32 gFreqMin = 100000;
UInt32 gFreqMax = 60000000;

bool gInitHW = false;

bool gExitThread = false;
bool gThreadRunning = false;

//---------------------------------------------------------------------------

ref class ManagedGlobals
{
  public:
    static GUI ^gGUI = nullptr;
    static RegistryKey ^gKey = nullptr;
};

//---------------------------------------------------------------------------

void (*ExtIOCallback)(int, int, float, void *) = 0;

static void SetRate(UInt32);
static void UpdateRate(UInt32);
static void SetCorr(Int32);

//---------------------------------------------------------------------------

DWORD WINAPI GeneratorThreadProc(__in LPVOID lpParameter)
{
  unsigned long size = 0;

  while(!gExitThread)
  {
    SleepEx(1, FALSE);
    if(gExitThread) break;

    ioctlsocket(gSock[1], FIONREAD, &size);

    while(size >= 4096)
    {
      recv(gSock[1], gBuffer, 4096, 0);

      if(ExtIOCallback) (*ExtIOCallback)(512, 0, 0.0, gBuffer);

      ioctlsocket(gSock[1], FIONREAD, &size);
    }
  }
  gExitThread = false;
  gThreadRunning = false;
  return 0;
}

//---------------------------------------------------------------------------

static void StopThread()
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

static void StartThread()
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
  String ^addrString;
  UInt32 rateIndex = 2;
  Int32 corr = 0;

  type = 7;

  strcpy(name, "Red Pitaya SDR TRX");
  strcpy(model, "");

  if(!gInitHW)
  {
    ManagedGlobals::gKey = Registry::CurrentUser->OpenSubKey("Software\\ExtIO_RedPitaya_TRX", true);
    if(!ManagedGlobals::gKey)
    {
      ManagedGlobals::gKey = Registry::CurrentUser->CreateSubKey("Software\\ExtIO_RedPitaya_TRX");
      ManagedGlobals::gKey->SetValue("IP Address", TCP_ADDR);
      ManagedGlobals::gKey->SetValue("Sample Rate", rateIndex);
      ManagedGlobals::gKey->SetValue("Freq. Corr.", corr);
    }

    ManagedGlobals::gGUI = gcnew GUI;
    addrString = ManagedGlobals::gKey->GetValue("IP Address")->ToString();
    ManagedGlobals::gGUI->addrValue->Text = addrString;

    rateIndex = Convert::ToUInt32(ManagedGlobals::gKey->GetValue("Sample Rate", 1));
    if(rateIndex < 0 || rateIndex > 4) rateIndex = 2;
    ManagedGlobals::gGUI->rateValue->SelectedIndex = rateIndex;
    ManagedGlobals::gGUI->rateCallback = UpdateRate;

    corr = Convert::ToInt32(ManagedGlobals::gKey->GetValue("Freq. Corr.", 0));
    if(corr < -100 || corr > 100) corr = 0;
    ManagedGlobals::gGUI->corrValue->Value = corr;
    ManagedGlobals::gGUI->corrCallback = SetCorr;

    SetRate(rateIndex);
    SetCorr(corr);

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
  String ^addrString;
  char *buffer;
  UInt32 command;
  int i;

  if(!gInitHW) return 0;

  WSAStartup(MAKEWORD(2, 2), &wsaData);

  addrString = ManagedGlobals::gGUI->addrValue->Text;
  ManagedGlobals::gKey->SetValue("IP Address", addrString);

  buffer = (char*)Marshal::StringToHGlobalAnsi(addrString).ToPointer();

  for(i = 0; i < 2; ++i)
  {
    gSock[i] = socket(AF_INET, SOCK_STREAM, 0);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(buffer);
    addr.sin_port = htons(TCP_PORT);

    connect(gSock[i], (struct sockaddr *)&addr, sizeof(addr));

    command = i;
    send(gSock[i], (char *)&command, 4, 0);
  }

  Marshal::FreeHGlobal(IntPtr(buffer));

  StopThread();
  gFreq = LOfreq;
  SetRate(ManagedGlobals::gGUI->rateValue->SelectedIndex);
  StartThread();

  return 512;
}

//---------------------------------------------------------------------------

extern "C"
void EXTIO_API StopHW()
{
  StopThread();

  closesocket(gSock[1]);
  closesocket(gSock[0]);

  gSock[1] = -1;
  gSock[0] = -1;

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
  int rc = 0;
  UInt32 buffer = 0;

  gFreq = LOfreq;

  // check limits
  if(gFreq < gFreqMin)
  {
    gFreq = gFreqMin;
    rc = -gFreqMin;
  }
  else if(gFreq > gFreqMax)
  {
    gFreq = gFreqMax;
    rc = gFreqMax;
  }

  if(gFreq != LOfreq && ExtIOCallback) (*ExtIOCallback)(-1, 101, 0.0, 0);

  buffer = (long)floor(gFreq*(1.0 + gCorr*1.0e-6) + 0.5);
  if(gSock[0] > 0) send(gSock[0], (char *)&buffer, 4, 0);

  return rc;
}

//---------------------------------------------------------------------------

static void SetRate(UInt32 rateIndex)
{
  switch(rateIndex)
  {
    case 0: gRate = 20000; gFreqMin = 10000; break;
    case 1: gRate = 50000; gFreqMin = 25000; break;
    case 2: gRate = 100000; gFreqMin = 50000; break;
    case 3: gRate = 250000; gFreqMin = 125000; break;
    case 4: gRate = 500000; gFreqMin = 250000; break;
  }

  if(ManagedGlobals::gKey) ManagedGlobals::gKey->SetValue("Sample Rate", rateIndex);

  rateIndex |= 1<<28;
  if(gSock[0] > 0) send(gSock[0], (char *)&rateIndex, 4, 0);

  SetHWLO(gFreq);
}

//---------------------------------------------------------------------------

static void UpdateRate(UInt32 rateIndex)
{
  SetRate(rateIndex);
  if(ExtIOCallback) (*ExtIOCallback)(-1, 100, 0.0, 0);
}

//---------------------------------------------------------------------------

static void SetCorr(Int32 corr)
{
  gCorr = corr;
  if(ManagedGlobals::gKey) ManagedGlobals::gKey->SetValue("Freq. Corr.", corr);
  SetHWLO(gFreq);
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
  return gRate;
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
