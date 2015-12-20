/*
 *  SoapyRedPitaya: Soapy SDR plug-in for Red Pitaya SDR transceiver
 *  Copyright (C) 2015  Pavel Demin
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <sstream>
#include <stdexcept>

#include <stdint.h>
#include <string.h>
#include <unistd.h>

#if defined(_WIN32) || defined (__CYGWIN__)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "SoapySDR/Device.hpp"
#include "SoapySDR/Registry.hpp"

#if defined(__APPLE__) || defined(__MACH__)
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL SO_NOSIGPIPE
#endif
#endif

using namespace std;

/***********************************************************************
 * Device interface
 **********************************************************************/

class SoapyRedPitaya : public SoapySDR::Device
{
public:
    SoapyRedPitaya(const SoapySDR::Kwargs &args):
        _addr("192.168.1.100"), _port(1001),
        _freq(6.0e5), _rate(1.0e5), _corr(0.0)
    {
        #if defined(_WIN32) || defined (__CYGWIN__)
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif

        _sockets[0] = -1;
        _sockets[1] = -1;

        if(args.count("addr")) _addr = args.at("addr");
        if(args.count("port")) stringstream(args.at("port")) >> _port;
    }

    ~SoapyRedPitaya()
    {
        #if defined(_WIN32) || defined (__CYGWIN__)
        WSACleanup();
        #endif
    }

    /*******************************************************************
     * Identification API
     ******************************************************************/

    string getDriverKey() const
    {
        return "redpitaya";
    }

    string getHardwareKey() const
    {
        return "redpitaya";
    }

    SoapySDR::Kwargs getHardwareInfo() const
    {
        SoapySDR::Kwargs info;
        return info;
    }

    /*******************************************************************
     * Channels API
     ******************************************************************/

    size_t getNumChannels(const int direction) const
    {
        return 1;
    }

    bool getFullDuplex(const int direction, const size_t channel) const
    {
        return true;
    }

    /*******************************************************************
     * Stream API
     ******************************************************************/

    vector<string> getStreamFormats(const int direction, const size_t channel) const
    {
        vector<string> formats;
        formats.push_back("CF32");
        return formats;
    }

    string getNativeStreamFormat(const int direction, const size_t channel, double &fullScale) const
    {
        fullScale = 1.0;
        return "CF32";
    }

    SoapySDR::ArgInfoList getStreamArgsInfo(const int direction, const size_t channel) const
    {
        SoapySDR::ArgInfoList streamArgs;
        return streamArgs;
    }

    SoapySDR::Stream *setupStream(
        const int direction,
        const string &format,
        const vector<size_t> &channels = vector<size_t>(),
        const SoapySDR::Kwargs &args = SoapySDR::Kwargs())
    {
        stringstream message;
        struct sockaddr_in addr;
        uint32_t command;

        if(format != "CF32") throw runtime_error("setupStream invalid format " + format);

        if(direction == SOAPY_SDR_RX)
        {
            command = 0;
        }

        if(direction == SOAPY_SDR_TX)
        {
            command = 2;
        }

        for(size_t i = 0; i < 2; ++i)
        {
            if((_sockets[i] = ::socket(AF_INET, SOCK_STREAM, 0)) < 0)
            {
                throw runtime_error("SoapyRedPitaya could not create TCP socket");
            }

            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(_addr.c_str());
            addr.sin_port = htons(_port);

            if(::connect(_sockets[i], (struct sockaddr *)&addr, sizeof(addr)) < 0)
            {
                message << "SoapyRedPitaya could not connect to " << _addr << ":" << _port;
                throw runtime_error(message.str());
            }

            sendCommand(_sockets[i], command);

            ++command;
        }

        return 0;
    }

    void closeStream(SoapySDR::Stream *stream)
    {
        ::close(_sockets[1]);
        _sockets[1] = -1;
        ::close(_sockets[0]);
        _sockets[0] = -1;
    }

    size_t getStreamMTU(SoapySDR::Stream *stream) const
    {
        return 2048;
    }

    int activateStream(
        SoapySDR::Stream *stream,
        const int flags = 0,
        const long long timeNs = 0,
        const size_t numElems = 0)
    {
        return 0;
    }

    int deactivateStream(
        SoapySDR::Stream *stream,
        const int flags = 0,
        const long long timeNs = 0)
    {
        return 0;
    }

    int readStream(
        SoapySDR::Stream *stream,
        void * const *buffs,
        const size_t numElems,
        int &flags,
        long long &timeNs,
        const long timeoutUs = 100000)
    {
        unsigned long size = 0;
        struct timeval timeout;

        #if defined(_WIN32) || defined (__CYGWIN__)
        ::ioctlsocket(_sockets[1], FIONREAD, &size);
        #else
        ::ioctl(_sockets[1], FIONREAD, &size);
        #endif

        if(size < 8 * numElems)
        {
            timeout.tv_sec = timeoutUs / 1000000;
            timeout.tv_usec = timeoutUs % 1000000;

            ::select(0, 0, 0, 0, &timeout);

            #if defined(_WIN32) || defined (__CYGWIN__)
            ::ioctlsocket(_sockets[1], FIONREAD, &size);
            #else
            ::ioctl(_sockets[1], FIONREAD, &size);
            #endif
        }

        if(size < 8 * numElems) return SOAPY_SDR_TIMEOUT;

        #if defined(_WIN32) || defined (__CYGWIN__)
        return ::recv(_sockets[1], (char *)buffs[0], 8 * numElems, MSG_WAITALL) / 8;
        #else
        return ::recv(_sockets[1], buffs[0], 8 * numElems, MSG_WAITALL) / 8;
        #endif
    }

    int writeStream(
        SoapySDR::Stream *stream,
        const void * const *buffs,
        const size_t numElems,
        int &flags,
        const long long timeNs = 0,
        const long timeoutUs = 100000)
    {
        ssize_t size;

        #if defined(_WIN32) || defined (__CYGWIN__)
        size = ::send(_sockets[1], (char *)buffs[0], 8 * numElems, 0);
        #else
        size = ::send(_sockets[1], buffs[0], 8 * numElems, MSG_NOSIGNAL);
        #endif

        if(size != 8 * numElems)
        {
            throw runtime_error("writeStream failed");
        }

        return numElems;
    }

    int readStreamStatus(
        SoapySDR::Stream *stream,
        size_t &chanMask,
        int &flags,
        long long &timeNs,
        const long timeoutUs)
    {
        return SOAPY_SDR_NOT_SUPPORTED;
    }

    /*******************************************************************
     * Frequency API
     ******************************************************************/

    void setFrequency(const int direction, const size_t channel, const string &name, const double frequency, const SoapySDR::Kwargs &args = SoapySDR::Kwargs())
    {
        uint32_t command = 0;

        if(name == "BB") return;
        if(name != "RF") throw runtime_error("setFrequency invalid name " + name);

        if(frequency < _rate / 2.0 || frequency > 6.0e7) return;

        command = (uint32_t)floor(frequency * (1.0 + _corr * 1.0e-6) + 0.5);

        sendCommand(_sockets[0], command);

        _freq = frequency;
    }

    double getFrequency(const int direction, const size_t channel, const string &name) const
    {
        if(name == "BB") return 0.0;
        if(name != "RF") throw runtime_error("getFrequency invalid name " + name);
        return _freq;
    }

    vector<string> listFrequencies(const int direction, const size_t channel) const
    {
        vector<string> names;
        names.push_back("RF");
        return names;
    }

    SoapySDR::RangeList getFrequencyRange(const int direction, const size_t channel, const string &name) const
    {
        if (name == "BB") return SoapySDR::RangeList(1, SoapySDR::Range(0.0, 0.0));
        if (name != "RF") throw runtime_error("getFrequencyRange invalid name " + name);
        return SoapySDR::RangeList(1, SoapySDR::Range(_rate / 2.0, 60.0e6));
    }

    /*******************************************************************
     * Sample Rate API
     ******************************************************************/

    void setSampleRate(const int direction, const size_t channel, const double rate)
    {
        uint32_t command = 0;

        if(2.0e4 == rate) command = 0;
        else if(5.0e4 == rate) command = 1;
        else if(1.0e5 == rate) command = 2;
        else if(2.5e5 == rate) command = 3;
        else if(5.0e5 == rate) command = 4;
        else if(1.25e6 == rate) command = 5;

        command |= 1<<28;
        sendCommand(_sockets[0], command);

        _rate = rate;
    }

    double getSampleRate(const int direction, const size_t channel) const
    {
        return _rate;
    }

    vector<double> listSampleRates(const int direction, const size_t channel) const
    {
        vector<double> rates;
        rates.push_back(2.0e4);
        rates.push_back(5.0e4);
        rates.push_back(1.0e5);
        rates.push_back(2.5e5);
        rates.push_back(5.0e5);
        rates.push_back(1.25e6);
        return rates;
    }

private:
    string _addr;
    unsigned short _port;
    double _freq, _rate, _corr;
    int _sockets[2];

    void sendCommand(int socket, uint32_t command)
    {
        ssize_t size;
        stringstream message;

        #if defined(_WIN32) || defined (__CYGWIN__)
        size = ::send(socket, (char *)&command, sizeof(command), 0);
        #else
        size = ::send(socket, &command, sizeof(command), MSG_NOSIGNAL);
        #endif

        if(size != sizeof(command))
        {
            message << "sendCommand failed: " << hex << command;
            throw runtime_error(message.str());
        }
    }
};

/***********************************************************************
 * Find available devices
 **********************************************************************/
SoapySDR::KwargsList findSoapyRedPitaya(const SoapySDR::Kwargs &args)
{
    vector<SoapySDR::Kwargs> results;
    return results;
}

/***********************************************************************
 * Make device instance
 **********************************************************************/
SoapySDR::Device *makeSoapyRedPitaya(const SoapySDR::Kwargs &args)
{
    return new SoapyRedPitaya(args);
}

/***********************************************************************
 * Registration
 **********************************************************************/
static SoapySDR::Registry registerSoapyRedPitaya("redpitaya", &findSoapyRedPitaya, &makeSoapyRedPitaya, SOAPY_SDR_ABI_VERSION);
