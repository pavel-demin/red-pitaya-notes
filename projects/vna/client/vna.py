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
from PyQt5.QtCore import QRegExp, QTimer, QSettings, Qt
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget, QFileDialog
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_VNA, QMainWindow = loadUiType('vna.ui')

class VNA(QMainWindow, Ui_VNA):

  max_size = 16383

  formatter = matplotlib.ticker.FuncFormatter(lambda x, pos: '%1.1fM' % (x * 1e-6) if abs(x) >= 1e6 else '%1.1fk' % (x * 1e-3) if abs(x) >= 1e3 else '%1.1f' % x if abs(x) >= 1e0 else '%1.1fm' % (x * 1e+3) if abs(x) >= 1e-3 else '%1.1fu' % (x * 1e+6) if abs(x) >= 1e-6 else '%1.1f' % x)

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
    self.sweep_stop = 60000
    self.sweep_size = 600
    self.xaxis, self.sweep_step = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size, retstep = True)
    self.xaxis *= 1000
    # buffer and offset for the incoming samples
    self.buffer = bytearray(16000 * VNA.max_size)
    self.offset = 0
    self.data = np.frombuffer(self.buffer, np.complex64)
    self.adc1 = np.zeros(VNA.max_size, np.complex64)
    self.adc2 = np.zeros(VNA.max_size, np.complex64)
    self.dac1 = np.zeros(VNA.max_size, np.complex64)
    self.open = np.zeros(VNA.max_size, np.complex64)
    self.short = np.zeros(VNA.max_size, np.complex64)
    self.load = np.zeros(VNA.max_size, np.complex64)
    self.dut = np.zeros(VNA.max_size, np.complex64)
    self.mode = 'dut'
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
    self.sweepFrame.setEnabled(False)
    self.dutSweep.setEnabled(False)
    self.connectButton.clicked.connect(self.start)
    self.writeButton.clicked.connect(self.write_cfg)
    self.readButton.clicked.connect(self.read_cfg)
    self.openSweep.clicked.connect(self.sweep_open)
    self.shortSweep.clicked.connect(self.sweep_short)
    self.loadSweep.clicked.connect(self.sweep_load)
    self.dutSweep.clicked.connect(self.sweep_dut)
    self.s1pButton.clicked.connect(self.write_s1p)
    self.startValue.valueChanged.connect(self.set_start)
    self.stopValue.valueChanged.connect(self.set_stop)
    self.sizeValue.valueChanged.connect(self.set_size)
    self.openPlot.clicked.connect(self.plot_open)
    self.shortPlot.clicked.connect(self.plot_short)
    self.loadPlot.clicked.connect(self.plot_load)
    self.dutPlot.clicked.connect(self.plot_dut)
    self.smithPlot.clicked.connect(self.plot_smith)
    self.impPlot.clicked.connect(self.plot_imp)
    self.rcPlot.clicked.connect(self.plot_rc)
    self.swrPlot.clicked.connect(self.plot_swr)
    self.rlPlot.clicked.connect(self.plot_rl)
    # create timer
    self.startTimer = QTimer(self)
    self.startTimer.timeout.connect(self.timeout)

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
    self.sweepFrame.setEnabled(False)
    self.dutSweep.setEnabled(False)

  def timeout(self):
    self.display_error('timeout')

  def connected(self):
    self.startTimer.stop()
    self.idle = False
    self.set_start(self.startValue.value())
    self.set_stop(self.stopValue.value())
    self.set_size(self.sizeValue.value())
    self.connectButton.setText('Disconnect')
    self.connectButton.setEnabled(True)
    self.sweepFrame.setEnabled(True)
    self.dutSweep.setEnabled(True)

  def read_data(self):
    size = self.socket.bytesAvailable()
    if self.offset + size < 16000 * self.sweep_size:
      self.buffer[self.offset:self.offset + size] = self.socket.read(size)
      self.offset += size
    else:
      self.buffer[self.offset:16000 * self.sweep_size] = self.socket.read(16000 * self.sweep_size - self.offset)
      self.offset = 0
      for i in range(0, self.sweep_size):
        start = i * 500
        stop = start + 500
        self.adc1[i] = np.fft.fft(self.data[0::4][start:stop])[5]
        self.adc2[i] = np.fft.fft(self.data[1::4][start:stop])[5]
        self.dac1[i] = np.fft.fft(self.data[2::4][start:stop])[5]
      getattr(self, self.mode)[0:self.sweep_size] = np.divide(self.adc1[0:self.sweep_size], self.dac1[0:self.sweep_size])
      self.sweepFrame.setEnabled(True)
      self.selectFrame.setEnabled(True)
      getattr(self, 'plot_%s' % self.mode)()

  def display_error(self, socketError):
    self.startTimer.stop()
    if socketError == 'timeout':
      QMessageBox.information(self, 'VNA', 'Error: connection timeout.')
    else:
      QMessageBox.information(self, 'VNA', 'Error: %s.' % self.socket.errorString())
    self.stop()

  def set_start(self, value):
    self.sweep_start = value
    self.xaxis, self.sweep_step = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size, retstep = True)
    self.xaxis *= 1000
    if self.idle: return
    self.socket.write(struct.pack('<I', 0<<28 | int(value * 1000)))

  def set_stop(self, value):
    self.sweep_stop = value
    self.xaxis, self.sweep_step = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size, retstep = True)
    self.xaxis *= 1000
    if self.idle: return
    self.socket.write(struct.pack('<I', 1<<28 | int(value * 1000)))

  def set_size(self, value):
    self.sweep_size = value
    self.xaxis, self.sweep_step = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size, retstep = True)
    self.xaxis *= 1000
    if self.idle: return
    self.socket.write(struct.pack('<I', 2<<28 | int(value)))

  def sweep(self):
    if self.idle: return
    self.sweepFrame.setEnabled(False)
    self.selectFrame.setEnabled(False)
    self.socket.write(struct.pack('<I', 3<<28))

  def sweep_open(self):
    self.mode = 'open'
    self.sweep()

  def sweep_short(self):
    self.mode = 'short'
    self.sweep()

  def sweep_load(self):
    self.mode = 'load'
    self.sweep()

  def sweep_dut(self):
    self.mode = 'dut'
    self.sweep()

  def impedance(self):
    return 50.0 * (self.open[0:self.sweep_size] - self.load[0:self.sweep_size]) * (self.dut[0:self.sweep_size] - self.short[0:self.sweep_size]) / ((self.load[0:self.sweep_size] - self.short[0:self.sweep_size]) * (self.open[0:self.sweep_size] - self.dut[0:self.sweep_size]))

  def gamma(self):
    z = self.impedance()
    return (z - 50.0)/(z + 50.0)

  def plot_magphase(self, data):
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(top = 0.98, right = 0.88)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(VNA.formatter)
    axes1.yaxis.set_major_formatter(VNA.formatter)
    axes1.tick_params('y', color = 'blue', labelcolor = 'blue')
    axes1.yaxis.label.set_color('blue')
    axes1.plot(self.xaxis, np.absolute(data), color = 'blue')
    axes2 = axes1.twinx()
    axes2.spines['left'].set_color('blue')
    axes2.spines['right'].set_color('red')
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Magnitude')
    axes2.set_ylabel('Phase angle')
    axes2.tick_params('y', color = 'red', labelcolor = 'red')
    axes2.yaxis.label.set_color('red')
    axes2.plot(self.xaxis, np.angle(data, deg = True), color = 'red')
    self.canvas.draw()

  def plot_fft(self):
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(top = 0.98, right = 0.88)
    axes = self.figure.add_subplot(111)
    axes.cla()
    w = np.kaiser(500, 12)
    axes.plot(np.fft.fftfreq(500), 20*np.log10(np.fft.fft(self.data[0::4][100*500:101*500] * w)))
    self.canvas.draw()

  def plot_open(self):
    self.plot_magphase(self.open[0:self.sweep_size])

  def plot_short(self):
    self.plot_magphase(self.short[0:self.sweep_size])

  def plot_load(self):
    self.plot_magphase(self.load[0:self.sweep_size])

  def plot_dut(self):
    self.plot_magphase(self.dut[0:self.sweep_size])

  def plot_smith(self):
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(top = 0.90, right = 0.90)
    axes = self.figure.add_subplot(111, projection = 'smith', axes_radius = 0.55, axes_scale = 50.0)
    axes.cla()
    axes.plot(self.impedance())
    self.canvas.draw()

  def plot_imp(self):
    self.plot_magphase(self.impedance())

  def plot_rc(self):
    self.plot_magphase(self.gamma())

  def plot_swr(self):
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(top = 0.98, right = 0.88)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(VNA.formatter)
    axes1.yaxis.set_major_formatter(VNA.formatter)
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('SWR')
    magnitude = np.absolute(self.gamma())
    swr = np.maximum(1.0, np.minimum(100.0, (1.0 + magnitude) / np.maximum(1.0e-20, 1.0 - magnitude)))
    axes1.plot(self.xaxis, swr, color = 'blue')
    self.canvas.draw()

  def plot_rl(self):
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(top = 0.98, right = 0.88)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(VNA.formatter)
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Return loss, dB')
    magnitude = np.absolute(self.gamma())
    axes1.plot(self.xaxis, 20.0 * np.log10(magnitude), color = 'blue')
    self.canvas.draw()

  def write_cfg(self):
    name = QFileDialog.getSaveFileName(self, 'Write configuration settings', '.', '*.ini')
    settings = QSettings(name[0], QSettings.IniFormat)
    self.write_cfg_settings(settings)

  def read_cfg(self):
    name = QFileDialog.getOpenFileName(self, 'Read configuration settings', '.', '*.ini')
    settings = QSettings(name[0], QSettings.IniFormat)
    self.read_cfg_settings(settings)

  def write_cfg_settings(self, settings):
    settings.setValue('start', self.startValue.value())
    settings.setValue('stop', self.stopValue.value())
    size = self.sizeValue.value()
    settings.setValue('size', size)
    for i in range(0, size):
      settings.setValue('open_real_%d' % i, float(self.open.real[i]))
      settings.setValue('open_imag_%d' % i, float(self.open.imag[i]))
    for i in range(0, size):
      settings.setValue('short_real_%d' % i, float(self.short.real[i]))
      settings.setValue('short_imag_%d' % i, float(self.short.imag[i]))
    for i in range(0, size):
      settings.setValue('load_real_%d' % i, float(self.load.real[i]))
      settings.setValue('load_imag_%d' % i, float(self.load.imag[i]))
    for i in range(0, size):
      settings.setValue('dut_real_%d' % i, float(self.dut.real[i]))
      settings.setValue('dut_imag_%d' % i, float(self.dut.imag[i]))

  def read_cfg_settings(self, settings):
    self.startValue.setValue(settings.value('start', 100, type = int))
    self.stopValue.setValue(settings.value('stop', 60000, type = int))
    size = settings.value('size', 600, type = int)
    self.sizeValue.setValue(size)
    for i in range(0, size):
      real = settings.value('open_real_%d' % i, 0.0, type = float)
      imag = settings.value('open_imag_%d' % i, 0.0, type = float)
      self.open[i] = real + 1.0j * imag
    for i in range(0, size):
      real = settings.value('short_real_%d' % i, 0.0, type = float)
      imag = settings.value('short_imag_%d' % i, 0.0, type = float)
      self.short[i] = real + 1.0j * imag
    for i in range(0, size):
      real = settings.value('load_real_%d' % i, 0.0, type = float)
      imag = settings.value('load_imag_%d' % i, 0.0, type = float)
      self.load[i] = real + 1.0j * imag
    for i in range(0, size):
      real = settings.value('dut_real_%d' % i, 0.0, type = float)
      imag = settings.value('dut_imag_%d' % i, 0.0, type = float)
      self.dut[i] = real + 1.0j * imag

  def write_s1p(self):
    name = QFileDialog.getSaveFileName(self, 'Write s1p file', '.', '*.s1p')
    fh = open(name[0], 'w')
    gamma = self.gamma()
    size = self.sizeValue.value()
    fh.write('# GHz S MA R 50\n')
    for i in range(0, size):
      fh.write('0.0%.8d   %8.6f %7.2f\n' % (self.xaxis[i], np.absolute(gamma[i]), np.angle(gamma[i], deg = True)))
    fh.close()

app = QApplication(sys.argv)
window = VNA()
window.show()
sys.exit(app.exec_())
