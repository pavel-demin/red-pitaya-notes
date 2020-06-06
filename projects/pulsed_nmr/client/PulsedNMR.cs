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

    public void SendCommand(long code, long data)
    {
      if(s == null) return;
      byte[] buffer = BitConverter.GetBytes(code << 60 | data);
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

    public void ClearPulses()
    {
      SendCommand(4, 0);
    }

    public void AddDelay(long width)
    {
      SendCommand(5, width - 4);
    }

    public void AddPulse(int level, int phase, long width)
    {
      SendCommand(5, width);
      SendCommand(6, (phase << 16) + level);
    }

    public float[] RecieveData(long size)
    {
      byte[] buffer = new byte[65536];
      float[] result = new float[size * 4];
      long offset = 0;
      int i, n, current, previous = 0;
      if(s == null) return result;
      SendCommand(7, size);
      while(offset < result.LongLength)
      {
        try
        {
          n = buffer.Length;
          if(n > (result.LongLength - offset) * 4) n = (int)(result.LongLength - offset) * 4;
          current = s.Receive(buffer, previous, n - previous, SocketFlags.None);
        }
        catch
        {
          Disconnect();
          break;
        }
        if(current == 0) break;
        n = current + previous;
        for(i = 0; i < n / 4; ++i)
        {
          result[offset + i] = BitConverter.ToSingle(buffer, i * 4);
        }
        previous = n % 4;
        if(previous > 0) Buffer.BlockCopy(buffer, n - previous, buffer, 0, previous);
        offset += n / 4;
      }
      return result;
    }
  }
}
