#!/usr/bin/env python

# GNU Radio blocks for the Red Pitaya transceiver
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
import numpy
import struct
import socket
from gnuradio import gr, blocks

class sdr_transceiver_rx(gr.sync_block):
  '''Red Pitaya sdr_transceiver_rx'''
  def __init__(self, addr, port, rx_rate, rx_freq, tx_freq, corr):
    gr.sync_block.__init__(
      self,
      name = "sdr_transceiver_rx",
      in_sig = [],
      out_sig = [numpy.complex64]
    )
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.connect((addr,port))
    self.set_rx_rate(rx_rate)
    self.set_rx_freq(rx_freq, corr)
    self.set_tx_freq(tx_freq, corr)

  def set_rx_rate(self, rate):
    if rate in sdr_transceiver_rx.rx_rates:
      code = sdr_transceiver_rx.rx_rates[rate]
      self.sock.send(struct.pack('<I', 1<<28 | code))
    else:
      raise ValueError("acceptable sample rates are 50k, 100k, 250k, 500k")

  def set_rx_freq(self, freq, corr):
    self.sock.send(struct.pack('<I', 0<<28 | int((1.0 + 1e-6 * corr) * freq)))

  def set_tx_freq(self, freq, corr):
    self.sock.send(struct.pack('<I', 2<<28 | int((1.0 + 1e-6 * corr) * freq)))

  def set_ptt(self, on):
    self.sock.send(struct.pack('<I', (3<<28, 4<<28)[on == False]))

  def work(self, input_items, output_items):
    data = self.sock.recv(len(output_items[0]) * 8, socket.MSG_WAITALL)
    output_items[0][:] = numpy.fromstring(data, numpy.complex64)
    return len(output_items[0])

sdr_transceiver_rx.rx_rates={50000:0, 100000:1, 250000:2, 500000:3}

class sdr_transceiver_tx(gr.sync_block):
  '''Red Pitaya sdr_transceiver_tx'''
  def __init__(self, addr, port):
    gr.sync_block.__init__(
      self,
      name = "sdr_transceiver_rx",
      in_sig = [numpy.complex64],
      out_sig = []
    )
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.connect((addr,port))

  def work(self, input_items, output_items):
    self.sock.send(input_items[0].tostring())
    return len(input_items[0])
