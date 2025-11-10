/*
compilation command:
C:\Windows\Microsoft.NET\Framework\v2.0.50727\csc.exe /target:library PulsedNMR.cs
*/

using System;
using System.Net.Sockets;
using System.Windows.Forms;

namespace PulsedNMR
{
  public class Client
  {
    private Socket s;

    public Client()
    {
      s = null;
    }

    public void Connect(String address)
    {
      if(s != null) return;
      try
      {
        s = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
        s.ReceiveTimeout = 1000;
        s.Connect(address, 1001);
      }
      catch(Exception ex)
      {
        Disconnect();
        MessageBox.Show(ex.Message);
      }
    }

    public bool Connected()
    {
      return s != null;
    }

    public void Disconnect()
    {
      if(s == null) return;
      try
      {
        s.Shutdown(SocketShutdown.Both);
      }
      catch
      {
      }
      finally
      {
        s.Close();
        s = null;
      }
    }

    public void SendCommand(double code, double data)
    {
      if(s == null) return;
      byte[] buffer = BitConverter.GetBytes((long)code << 60 | (long)data);
      try
      {
        s.Send(buffer);
      }
      catch
      {
        Disconnect();
      }
    }

    public void SetFreqRX(int freq)
    {
      SendCommand(0, freq);
    }

    public void SetFreqTX(int freq)
    {
      SendCommand(1, freq);
    }

    public void SetRateRX(int rate)
    {
      SendCommand(2, rate);
    }

    public void SetLevelTX(int level)
    {
      SendCommand(3, level);
    }

    public void SetPin(int pin)
    {
      SendCommand(4, pin);
    }

    public void ClearPin(int pin)
    {
      SendCommand(5, pin);
    }

    public void SetDAC(int data)
    {
      SendCommand(6, data);
    }

    public void ClearEvents()
    {
      SendCommand(7, 0);
    }

    public void AddEventTX(double delay, int sync, int gate, double level, double tx_phase, double rx_phase)
    {
      double lvl = Math.Floor(level / 100.0 * 32766 + 0.5);
      double txp = Math.Floor(tx_phase / 360.0 * 0x3FFFFFFF + 0.5);
      double rxp = Math.Floor(rx_phase / 360.0 * 0x3FFFFFFF + 0.5);
      SendCommand(8, (long)lvl << 44 | (long)gate << 41 | (long)sync << 40 | (long)(delay - 1));
      SendCommand(9, (long)rxp << 30 | (long)txp);
    }

    public void AddEventRX(double size, int read)
    {
      SendCommand(10, (long)read << 40 | (long)(size - 1));
    }

    public int[] RecieveData(int size)
    {
      int n, offset = 0, limit = size * 16;
      byte[] buffer;
      int[] result;
      try
      {
        buffer = new byte[65536];
        result = new int[size * 4];
      }
      catch(Exception ex)
      {
        MessageBox.Show(ex.Message);
        return new int[0];
      }
      if(s == null) return result;
      SendCommand(11, size);
      while(offset < limit)
      {
        try
        {
          n = limit - offset;
          if(n > buffer.Length) n = buffer.Length;
          n = s.Receive(buffer, 0, n, SocketFlags.None);
        }
        catch
        {
          Disconnect();
          break;
        }
        if(n == 0) break;
        Buffer.BlockCopy(buffer, 0, result, offset, n);
        offset += n;
      }
      return result;
    }
  }
}
