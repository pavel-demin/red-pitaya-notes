#!/usr/bin/env python

# Control program for the Red Pitaya vector network analyzer
# Copyright (C) 2016  Pavel Demin
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import struct

import numpy as np

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
import smithplot

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp, QTimer, Qt
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_VNA, QMainWindow = loadUiType('vna.ui')

class VNA(QMainWindow, Ui_VNA):
  def __init__(self):
    super(VNA, self).__init__()
    self.setupUi(self)
    # IP address validator
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    # state variable
    self.idle = True
    # sweep parameters
    self.sweep_start = 100
    self.sweep_step = 100
    self.sweep_size = 600
    self.xaxis = np.linspace(self.sweep_start, self.sweep_start * (self.sweep_size - 1), self.sweep_size)
    # buffer and offset for the incoming samples
    self.buffer = bytearray(32000 * 1000)
    self.offset = 0
    self.data = np.frombuffer(self.buffer, np.complex64)
    self.adc = np.zeros(1000, np.complex64)
    self.dac = np.zeros(1000, np.complex64)
    self.open = np.zeros(1000, np.complex64)
    self.short = np.zeros(1000, np.complex64)
    self.load = np.zeros(1000, np.complex64)
    self.dut = np.zeros(1000, np.complex64)
    # create figure
    self.figure = Figure()
    self.figure.set_facecolor('none')
    self.canvas = FigureCanvas(self.figure)
    self.plotLayout.addWidget(self.canvas)
    # create navigation toolbar
    self.toolbar = NavigationToolbar(self.canvas, self.plotWidget, False)
    # remove subplots action
    actions = self.toolbar.actions()
    self.toolbar.removeAction(actions[7])
    self.plotLayout.addWidget(self.toolbar)
    # create TCP socket
    self.socket = QTcpSocket(self)
    self.socket.connected.connect(self.connected)
    self.socket.readyRead.connect(self.read_data)
    self.socket.error.connect(self.display_error)
    # connect signals from buttons and boxes
    self.buttons_enabled(False)
    self.connectButton.clicked.connect(self.start)
    self.openButton.clicked.connect(self.sweep_open)
    self.shortButton.clicked.connect(self.sweep_short)
    self.loadButton.clicked.connect(self.sweep_load)
    self.dutButton.clicked.connect(self.sweep_dut)
    self.startValue.valueChanged.connect(self.set_start)
    self.stepValue.valueChanged.connect(self.set_step)
    self.sizeValue.valueChanged.connect(self.set_size)
    self.smithButton.clicked.connect(self.plot_smith)
    self.magButton.clicked.connect(self.plot_mag)
    self.argButton.clicked.connect(self.plot_arg)
    # create timer
    self.startTimer = QTimer(self)
    self.startTimer.timeout.connect(self.timeout)

  def buttons_enabled(self, value):
    self.openButton.setEnabled(value)
    self.shortButton.setEnabled(value)
    self.loadButton.setEnabled(value)
    self.dutButton.setEnabled(value)

  def start(self):
    if self.idle:
      self.connectButton.setEnabled(False)
      self.socket.connectToHost(self.addrValue.text(), 1001)
      self.startTimer.start(5000)
    else:
      self.stop()

  def stop(self):
    self.idle = True
    self.socket.abort()
    self.offset = 0
    self.connectButton.setText('Connect')
    self.connectButton.setEnabled(True)
    self.buttons_enabled(False)

  def timeout(self):
    self.display_error('timeout')

  def connected(self):
    self.startTimer.stop()
    self.idle = False
    self.set_start(self.startValue.value())
    self.set_step(self.stepValue.value())
    self.set_size(self.sizeValue.value())
    self.connectButton.setText('Disconnect')
    self.connectButton.setEnabled(True)
    self.buttons_enabled(True)

  def read_data(self):
    size = self.socket.bytesAvailable()
    if self.offset + size < 32000 * self.sweep_size:
      self.buffer[self.offset:self.offset + size] = self.socket.read(size)
      self.offset += size
    else:
      self.buffer[self.offset:32000 * self.sweep_size] = self.socket.read(32000 * self.sweep_size - self.offset)
      self.offset = 0
      for i in range(0, self.sweep_size):
        start = i * 1000
        stop = start + 1000
        self.adc[i] = np.fft.fft(self.data[0::4][start:stop])[10]
        self.dac[i] = np.fft.fft(self.data[2::4][start:stop])[10]
      getattr(self, self.mode)[0:self.sweep_size] = np.divide(self.adc[0:self.sweep_size], self.dac[0:self.sweep_size])
      self.buttons_enabled(True)

  def display_error(self, socketError):
    self.startTimer.stop()
    if socketError == 'timeout':
      QMessageBox.information(self, 'VNA', 'Error: connection timeout.')
    else:
      QMessageBox.information(self, 'VNA', 'Error: %s.' % self.socket.errorString())
    self.stop()

  def set_start(self, value):
    return

  def set_step(self, value):
    return

  def set_size(self, value):
    return

  def sweep(self):
    if self.idle: return
    self.socket.write(struct.pack('<I', 3<<28))

  def sweep_open(self):
    self.buttons_enabled(False)
    self.mode = 'open'
    self.sweep()

  def sweep_short(self):
    self.buttons_enabled(False)
    self.mode = 'short'
    self.sweep()

  def sweep_load(self):
    self.buttons_enabled(False)
    self.mode = 'load'
    self.sweep()


  def sweep_dut(self):
    self.buttons_enabled(False)
    self.mode = 'dut'
    self.sweep()

  def result(self):
    if self.mode == 'dut':
      return 50.0 * (self.open[0:self.sweep_size] - self.load[0:self.sweep_size]) * (self.dut[0:self.sweep_size] - self.short[0:self.sweep_size]) / ((self.load[0:self.sweep_size] - self.short[0:self.sweep_size]) * (self.open[0:self.sweep_size] - self.dut[0:self.sweep_size]))
    else:
      return getattr(self, self.mode)[0:self.sweep_size]

  def plot_smith(self):
    matplotlib.rcdefaults() 
    self.figure.clf()
    axes = self.figure.add_subplot(111, projection = 'smith', axes_scale = 50)
    axes.cla()
    axes.plot(self.result())
    self.canvas.draw()

  def plot_mag(self):
    matplotlib.rcdefaults() 
    self.figure.clf()
    axes = self.figure.add_subplot(111)
    axes.cla()
    axes.set_xlabel('kHz')
    axes.set_ylabel('Ohm')
    mkfunc = lambda x, pos: '%1.1fM' % (x * 1e-6) if x >= 1e6 else '%1.1fk' % (x * 1e-3) if x >= 1e3 else '%1.1f' % x if x >= 1e0 else '%1.1fm' % (x * 1e+3)
    mkformatter = matplotlib.ticker.FuncFormatter(mkfunc)
    axes.xaxis.set_major_formatter(mkformatter)
    axes.yaxis.set_major_formatter(mkformatter)
    axes.plot(self.xaxis, np.absolute(self.result()))
    self.canvas.draw()

  def plot_arg(self):
    matplotlib.rcdefaults() 
    self.figure.clf()
    axes = self.figure.add_subplot(111)
    axes.cla()
    axes.set_xlabel('kHz')
    axes.set_ylabel('degree')
    mkfunc = lambda x, pos: '%1.1fM' % (x * 1e-6) if x >= 1e6 else '%1.1fk' % (x * 1e-3) if x >= 1e3 else '%1.1f' % x if x >= 1e0 else '%1.1fm' % (x * 1e+3)
    mkformatter = matplotlib.ticker.FuncFormatter(mkfunc)
    axes.xaxis.set_major_formatter(mkformatter)
    axes.plot(self.xaxis, np.angle(self.result(), deg = True))
    self.canvas.draw()

app = QApplication(sys.argv)
window = VNA()
window.show()
sys.exit(app.exec_())
