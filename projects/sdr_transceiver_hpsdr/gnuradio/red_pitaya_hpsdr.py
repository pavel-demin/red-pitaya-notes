#!/usr/bin/env python

# GNU Radio blocks for the Red Pitaya transceiver compatible with HPSDR
# Copyright (C) 2015  Renzo Davoli
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import os
import math
import numpy
import struct
import socket
from gnuradio import gr, blocks

class source(gr.sync_block):
  '''Red Pitaya HPSDR Source'''

  rates = {48000:0, 96000:1, 192000:2, 384000:3}

  def __init__(self, addr, port, freq, rate, corr):
    gr.sync_block.__init__(
      self,
      name = "red_pitaya_hpsdr_source",
      in_sig = [],
      out_sig = [numpy.complex64]
    )
    self.ctrl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.ctrl_sock.connect((addr, port))
    self.ctrl_sock.send(struct.pack('<I', 0))
    self.data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.data_sock.connect((addr, port))
    self.data_sock.send(struct.pack('<I', 1))
    self.set_freq(freq, corr)
    self.set_rate(rate)

  def set_freq(self, freq, corr):
    self.ctrl_sock.send(struct.pack('<I', 0<<28 | int((1.0 + 1e-6 * corr) * freq)))

  def set_rate(self, rate):
    if rate in source.rates:
      code = source.rates[rate]
      self.ctrl_sock.send(struct.pack('<I', 1<<28 | code))
    else:
      raise ValueError("acceptable sample rates are 48k, 96k, 192k, 384k")

  def work(self, input_items, output_items):
    data = self.data_sock.recv(len(output_items[0]) * 8, socket.MSG_WAITALL)
    temp = numpy.fromstring(data, numpy.int32) / math.pow(2, 20)
    output_items[0][:] = numpy.fromstring(temp.astype(numpy.float32).tostring(), numpy.complex64)
    return len(output_items[0])

class sink(gr.sync_block):
  '''Red Pitaya HPSDR Sink'''

  def __init__(self, addr, port, freq, corr):
    gr.sync_block.__init__(
      self,
      name = "red_pitaya_hpsdr_sink",
      in_sig = [numpy.complex64],
      out_sig = []
    )
    self.ctrl_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.ctrl_sock.connect((addr, port))
    self.ctrl_sock.send(struct.pack('<I', 2))
    self.data_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.data_sock.connect((addr, port))
    self.data_sock.send(struct.pack('<I', 3))
    self.ptt = False
    self.set_freq(freq, corr)

  def set_freq(self, freq, corr):
    self.ctrl_sock.send(struct.pack('<I', 0<<28 | int((1.0 + 1e-6 * corr) * freq)))

  def set_ptt(self, on):
    if on and not self.ptt:
      self.ptt = True
      self.ctrl_sock.send(struct.pack('<I', 1<<28))
    elif not on and self.ptt:
      self.ptt = False
      self.ctrl_sock.send(struct.pack('<I', 2<<28))

  def work(self, input_items, output_items):
    if self.ptt:
      temp = numpy.fromstring(input_items[0].tostring(), numpy.float32) * math.pow(2, 31)
      self.data_sock.send(temp.astype(numpy.int32).tostring())
    return len(input_items[0])
