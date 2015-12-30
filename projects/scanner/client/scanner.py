#!/usr/bin/env python

# Control program for the Red Pitaya Scanning system
# Copyright (C) 2015  Pavel Demin
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
import matplotlib.cm as cm

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp, QTimer
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_Scanner, QMainWindow = loadUiType('scanner.ui')

class Scanner(QMainWindow, Ui_Scanner):
  def __init__(self):
    super(Scanner, self).__init__()
    self.setupUi(self)
    # IP address validator
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    # state variable
    self.idle = True
    # number of samples to show on the plot
    self.size = 512 * 512
    self.freq = 143.0
    # buffer and offset for the incoming samples
    self.buffer = bytearray(8 * self.size)
    self.offset = 0
    self.data = np.frombuffer(self.buffer, np.int32)
    # create figure
    figure = Figure()
    figure.set_facecolor('none')
    self.axes = figure.add_subplot(111)
    self.canvas = FigureCanvas(figure)
    self.plotLayout.addWidget(self.canvas)
    self.axes.axis((0.0, 512.0, 0.0, 512.0))
    x, y = np.meshgrid(np.linspace(0.0, 512.0, 513), np.linspace(0.0, 512.0, 513))
    z = x / 512.0 + y * 0.0
    self.mesh = self.axes.pcolormesh(x, y, z, cmap = cm.plasma)
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
    self.connectButton.clicked.connect(self.start)
    self.scanButton.clicked.connect(self.scan)
    self.periodValue.valueChanged.connect(self.set_period)
    self.trgtimeValue.valueChanged.connect(self.set_trgtime)
    self.shdelayValue.valueChanged.connect(self.set_shdelay)
    self.shtimeValue.valueChanged.connect(self.set_shtime)
    self.acqdelayValue.valueChanged.connect(self.set_acqdelay)
    self.samplesValue.valueChanged.connect(self.set_samples)
    # create timers
    self.startTimer = QTimer(self)
    self.startTimer.timeout.connect(self.timeout)
    self.meshTimer = QTimer(self)
    self.meshTimer.timeout.connect(self.update_mesh)

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
    self.scanButton.setEnabled(True)

  def timeout(self):
    self.startTimer.stop()
    self.display_error('timeout')

  def connected(self):
    self.startTimer.stop()
    self.idle = False
    self.set_period(self.periodValue.value())
    self.set_trgtime(self.trgtimeValue.value())
    self.set_shdelay(self.shdelayValue.value())
    self.set_shtime(self.shtimeValue.value())
    self.set_acqdelay(self.acqdelayValue.value())
    self.set_samples(self.samplesValue.value())
    self.connectButton.setText('Disconnect')
    self.connectButton.setEnabled(True)
    self.scanButton.setEnabled(True)

  def read_data(self):
    size = self.socket.bytesAvailable()
    if self.offset + size < 8 * self.size:
      self.buffer[self.offset:self.offset + size] = self.socket.read(size)
      self.offset += size
    else:
      self.meshTimer.stop()
      self.buffer[self.offset:8 * self.size] = self.socket.read(8 * self.size - self.offset)
      self.offset = 0
      self.update_mesh()
      self.scanButton.setEnabled(True)

  def display_error(self, socketError):
    if socketError == 'timeout':
      QMessageBox.information(self, 'Scanner', 'Error: connection timeout.')
    else:
      QMessageBox.information(self, 'Scanner', 'Error: %s.' % self.socket.errorString())
    self.stop()

  def set_period(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 0<<28 | int(value * self.freq)))

  def set_trgtime(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 1<<28 | int(value * self.freq)))

  def set_shdelay(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 2<<28 | int(value * self.freq)))

  def set_shtime(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 3<<28 | int(value * self.freq)))

  def set_acqdelay(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 4<<28 | int(value * self.freq)))

  def set_samples(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 5<<28 | int(value)))

  def scan(self):
    if self.idle: return
    self.scanButton.setEnabled(False)
    self.data[:] = np.zeros(2 * 512 * 512, np.int32)
    self.update_mesh()
    self.socket.write(struct.pack('<I', 6<<28))
    self.meshTimer.start(1000)

  def update_mesh(self):
    self.mesh.set_array(self.data[0::2]/self.samplesValue.value()/8192.0)
    self.canvas.draw()

app = QApplication(sys.argv)
window = Scanner()
window.show()
sys.exit(app.exec_())
