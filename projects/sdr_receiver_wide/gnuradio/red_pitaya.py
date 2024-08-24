#!/usr/bin/env python

import os
import numpy
import struct
import socket
from gnuradio import gr, blocks

class source(gr.hier_block2):
  '''Red Pitaya Source'''

  rates = {500000:125, 1250000:50, 2500000:25, 3125000:20,5208333:12 }

  def __init__(self, addr, port, rate, freq, corr, attu, pga):
    gr.hier_block2.__init__(
      self,
      name = "red_pitaya_source",
      input_signature = gr.io_signature(0, 0, 0),
      output_signature = gr.io_signature(1, 1, gr.sizeof_gr_complex)
    )
    self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.sock.connect((addr, port))
    fd = os.dup(self.sock.fileno())
    self.connect(blocks.file_descriptor_source(gr.sizeof_gr_complex, fd), self)
    self.set_rate(rate)
    self.set_freq(freq, corr)
    self.set_attu(attu)
    self.set_pga(pga)

  def set_rate(self, rate):
    if rate in source.rates:
      code = source.rates[rate]
      self.sock.send(struct.pack('<I', 0<<28 | code))
    else:
      raise ValueError("acceptable sample rates are 500k, 1250k, 2500k, 3125k")

  def set_freq(self, freq, corr):
    self.sock.send(struct.pack('<I', 1<<28 | int((1.0 + 1e-6 * corr) * freq)))
  def set_attu(self, attu):
    self.sock.send(struct.pack('<I', 3<<28 | 10 - attu))
  def set_pga(self, pga):
    self.sock.send(struct.pack('<I', 4<<28 | pga))
