#!/usr/bin/env python

import os
import mmap
import time
import numpy
import struct
from gnuradio import gr, blocks

class source(gr.sync_block):
  '''Red Pitaya Embedded Source'''

  rates = {24000:2500, 48000:1250, 96000:625}

  def __init__(self, chan, freq, rate, corr):
    gr.sync_block.__init__(
      self,
      name = "red_pitaya_source",
      in_sig = [],
      out_sig = [numpy.complex64]
    )
    file = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
    self.cfg = mmap.mmap(file, 4096, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40000000)
    self.sts = mmap.mmap(file, 4096, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40001000)
    self.buf = mmap.mmap(file, 8192, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40002000)
    self.cfg[0:1] = struct.pack('<B', 1)
    self.cfg[0:1] = struct.pack('<B', 0)
    self.set_freq(freq, corr)
    self.set_rate(rate)

  def set_freq(self, freq, corr):
    self.cfg[4:8] = struct.pack('<I', int((1.0 + 1e-6 * corr) * freq / 125.0e6 * (1<<30) + 0.5))

  def set_rate(self, rate):
    if rate in source.rates:
      self.cfg[8:12] = struct.pack('<I', source.rates[rate])
    else:
      raise ValueError("acceptable sample rates are 24k, 48k, 96k")

  def work(self, input_items, output_items):
    size = len(output_items[0])
    offset = 0
    while size > 0:
      if struct.unpack("<H", self.sts[0:2])[0] >= 2048:
        self.cfg[0:1] = struct.pack('<B', 1)
        self.cfg[0:1] = struct.pack('<B', 0)
      while struct.unpack("<H", self.sts[0:2])[0] < 1024: time.sleep(0.001)
      if size < 512:
        output_items[0][offset:offset+size] = numpy.fromstring(self.buf[0:8*size], numpy.complex64)
        size = 0
        offset = offset + size
      else:
        output_items[0][offset:offset+512] = numpy.fromstring(self.buf[0:4096], numpy.complex64)
        size = size - 512
        offset = offset + 512
    return len(output_items[0])

class sink(gr.sync_block):
  '''Red Pitaya Embedded Sink'''

  rates = {24000:2500, 48000:1250, 96000:625}

  def __init__(self, chan, freq, rate, corr, ptt):
    gr.sync_block.__init__(
      self,
      name = "red_pitaya_sink",
      in_sig = [numpy.complex64],
      out_sig = []
    )
    file = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
    self.cfg = mmap.mmap(file, 4096, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40000000)
    self.sts = mmap.mmap(file, 4096, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40001000)
    self.buf = mmap.mmap(file, 8192, mmap.MAP_SHARED, mmap.PROT_READ | mmap.PROT_WRITE, offset = 0x40004000)
    self.cfg[1:2] = struct.pack('<B', 1)
    self.cfg[1:2] = struct.pack('<B', 0)
    self.set_freq(freq, corr)
    self.set_rate(rate)
    if ptt:
      self.ptt = True
      self.cfg[2:4] = struct.pack('<H', 1)
    else:
      self.ptt = False
      self.cfg[2:4] = struct.pack('<H', 0)

  def set_freq(self, freq, corr):
    self.cfg[12:16] = struct.pack('<I', int((1.0 + 1e-6 * corr) * freq / 125.0e6 * (1<<30) + 0.5))

  def set_rate(self, rate):
    if rate in sink.rates:
      self.cfg[16:20] = struct.pack('<I', sink.rates[rate])
    else:
      raise ValueError("acceptable sample rates are 24k, 48k, 96k")

  def set_ptt(self, ptt):
    if ptt and not self.ptt:
      self.ptt = True
      self.cfg[2:4] = struct.pack('<H', 1)
    elif not ptt and self.ptt:
      self.ptt = False
      self.cfg[2:4] = struct.pack('<H', 0)

  def work(self, input_items, output_items):
    size = len(input_items[0])
    if not self.ptt: return size
    offset = 0
    while size > 0:
      while struct.unpack("<H", self.sts[2:4])[0] > 1024: time.sleep(0.001)
      if(struct.unpack("<H", self.sts[2:4])[0] == 0):
        self.buf[0:4096] = numpy.zeros(512, numpy.complex64).tostring()
      if size < 512:
        self.buf[0:8*size] = input_items[0][offset:offset+size].tostring()
        size = 0
        offset = offset + size
      else:
        self.buf[0:4096] = input_items[0][offset:offset+512].tostring()
        size = size - 512
        offset = offset + 512
    return len(input_items[0])
