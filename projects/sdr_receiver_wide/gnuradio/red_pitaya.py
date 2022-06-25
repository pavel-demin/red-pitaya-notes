#!/usr/bin/env python

import os
import numpy
import struct
import socket
from gnuradio import gr, blocks

class source(gr.hier_block2):
  '''Red Pitaya Source'''

  rates = {500000:125, 1250000:50, 2500000:25, 3125000:20}

  def __init__(self, addr, port, rate, freq1, freq2, corr):
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
    self.set_freq1(freq1, corr)
    self.set_freq2(freq2, corr)

  def set_rate(self, rate):
    if rate in source.rates:
      code = source.rates[rate]
      self.sock.send(struct.pack('<I', 0<<28 | code))
    else:
      raise ValueError("acceptable sample rates are 500k, 1250k, 2500k, 3125k")

  def set_freq1(self, freq, corr):
    self.sock.send(struct.pack('<I', 1<<28 | int((1.0 + 1e-6 * corr) * freq)))

  def set_freq2(self, freq, corr):
    self.sock.send(struct.pack('<I', 2<<28 | int((1.0 + 1e-6 * corr) * freq)))
