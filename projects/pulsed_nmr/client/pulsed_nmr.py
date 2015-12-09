#!/usr/bin/env python

import sys
import struct

import numpy as np

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp, QTimer
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_PulsedNMR, QMainWindow = loadUiType('pulsed_nmr.ui')

class PulsedNMR(QMainWindow, Ui_PulsedNMR):
  def __init__(self):
    super(PulsedNMR, self).__init__()
    self.setupUi(self)
    self.rateValue.addItems(['25', '50', '250', '500', '2500'])
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    fig = Figure()
    fig.set_facecolor('none')
    self.axes = fig.add_subplot(111)
    self.axes.grid()
    self.curve, = self.axes.plot(np.zeros(8192))
    x1, x2, y1, y2 = self.axes.axis()
    self.axes.axis((x1, x2, -0.1, 0.4))
    self.canvas = FigureCanvas(fig)
    self.plot.addWidget(self.canvas)
    self.canvas.draw()
    self.idle = True
    self.socket = QTcpSocket(self)
    self.socket.connected.connect(self.connected)
    self.socket.readyRead.connect(self.read_data)
    self.socket.error.connect(self.display_error)
    self.startButton.clicked.connect(self.start)
    self.freqValue.valueChanged.connect(self.set_freq)
    self.awidthValue.valueChanged.connect(self.set_awidth)
    self.deltaValue.valueChanged.connect(self.set_delta)
    self.rateValue.currentIndexChanged.connect(self.set_rate)
    self.timer = QTimer(self)
    self.timer.timeout.connect(self.fire)
    self.buffer = bytearray(65536)
    self.offset = 0

  def start(self):
    if self.idle:
      self.startButton.setEnabled(False)
      self.socket.connectToHost(self.addrValue.text(), 1001)
    else:
      self.idle = True
      self.timer.stop()
      self.socket.close()
      self.startButton.setText('Start')
      self.startButton.setEnabled(True)

  def connected(self):
    self.idle = False
    self.set_freq(self.freqValue.value())
    self.set_rate(self.rateValue.currentIndex())
    self.set_awidth(self.awidthValue.value())
    self.fire()
    self.timer.start(self.deltaValue.value())
    self.startButton.setText('Stop')
    self.startButton.setEnabled(True)

  def read_data(self):
    size = self.socket.bytesAvailable()
    if self.offset + size < 65536:
      self.buffer[self.offset:self.offset + size] = self.socket.read(size)
      self.offset += size
    else:
      self.buffer[self.offset:65536] = self.socket.read(65536 - self.offset)
      self.offset = 0
      data = np.frombuffer(self.buffer, np.complex64)
      self.curve.set_ydata(np.abs(data))
      self.canvas.draw()

  def display_error(self, socketError):
    if socketError == QAbstractSocket.RemoteHostClosedError:
      pass
    else:
      QMessageBox.information(self, 'PulsedNMR', 'Error: %s.' % self.socket.errorString())
    self.startButton.setText('Start')
    self.startButton.setEnabled(True)

  def set_freq(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 0<<28 | int(1.0e6 * value)))

  def set_rate(self, index):
    if self.idle: return
    self.socket.write(struct.pack('<I', 1<<28 | index))

  def set_awidth(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 2<<28 | int(1.0e1 * value)))

  def set_delta(self, value):
    if self.idle: return
    self.timer.stop()
    self.timer.start(value)

  def fire(self):
    if self.idle: return
    self.socket.write(struct.pack('<I', 3<<28))

app = QApplication(sys.argv)
window = PulsedNMR()
window.show()
sys.exit(app.exec_())
