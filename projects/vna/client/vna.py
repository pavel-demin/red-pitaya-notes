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
import warnings

import numpy as np

import matplotlib
matplotlib.use('Qt5Agg')
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar
from matplotlib.figure import Figure
from matplotlib.ticker import Formatter, FuncFormatter

from mpldatacursor import datacursor

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp, QTimer, QSettings, QDir, Qt
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget, QDialog, QFileDialog, QProgressDialog
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_VNA, QMainWindow = loadUiType('vna.ui')

def metric_prefix(x, pos = None):
  if abs(x) >= 1.0e6:
    return '%1.1fM' % (x * 1.0e-6)
  elif abs(x) >= 1.0e3:
    return '%1.1fk' % (x * 1.0e-3)
  elif abs(x) >= 1.0e0:
    return '%1.1f' % x
  elif abs(x) >= 1.0e-3:
    return '%1.1fm' % (x * 1e+3)
  elif abs(x) >= 1.0e-6:
    return '%1.1fu' % (x * 1e+6)
  else:
    return '%1.1f' % x

class GammaFormatter:
  def __init__(self, xaxis):
    self.xaxis = xaxis
  def __call__(self, x = None, y = None, ind = None, **kwargs):
    gamma = complex(x, y)
    z = 50.0 * (1.0 + gamma)/(1.0 - gamma)
    if z.imag >= 0.0:
      return u'Z: %s\u03A9 + j%s\u03A9\nFrequency: %sHz' % (metric_prefix(z.real), metric_prefix(z.imag), metric_prefix(self.xaxis[ind[0]]))
    else:
      return u'Z: %s\u03A9 \u2212 j%s\u03A9\nFrequency: %sHz' % (metric_prefix(z.real), metric_prefix(-z.imag), metric_prefix(self.xaxis[ind[0]]))

class VNA(QMainWindow, Ui_VNA):

  max_size = 16384

  def __init__(self):
    super(VNA, self).__init__()
    self.setupUi(self)
    # IP address validator
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    # state variables
    self.idle = True
    self.reading = False
    # sweep parameters
    self.sweep_start = 100
    self.sweep_stop = 60000
    self.sweep_size = 600
    self.xaxis, self.sweep_step = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size, retstep = True)
    self.xaxis *= 1000
    # buffer and offset for the incoming samples
    self.buffer = bytearray(32 * VNA.max_size)
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
    # initialize cursor
    self.cursor = None
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
    self.rateValue.addItems(['500', '50', '5', '0.5'])
    self.rateValue.currentIndexChanged.connect(self.set_rate)
    self.corrValue.valueChanged.connect(self.set_corr)
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
    self.connectButton.setText('Connect')
    self.connectButton.setEnabled(True)
    self.sweepFrame.setEnabled(False)
    self.selectFrame.setEnabled(True)
    self.dutSweep.setEnabled(False)

  def timeout(self):
    self.display_error('timeout')

  def connected(self):
    self.startTimer.stop()
    self.idle = False
    self.set_start(self.startValue.value())
    self.set_stop(self.stopValue.value())
    self.set_size(self.sizeValue.value())
    self.set_rate(self.rateValue.currentIndex())
    self.set_corr(self.corrValue.value())
    self.connectButton.setText('Disconnect')
    self.connectButton.setEnabled(True)
    self.sweepFrame.setEnabled(True)
    self.dutSweep.setEnabled(True)

  def read_data(self):
    if not self.reading:
      self.socket.readAll()
      return
    size = self.socket.bytesAvailable()
    self.progress.setValue((self.offset + size) / 32)
    limit = 32 * (self.sweep_size + 1)
    if self.offset + size < limit:
      self.buffer[self.offset:self.offset + size] = self.socket.read(size)
      self.offset += size
    else:
      self.buffer[self.offset:limit] = self.socket.read(limit - self.offset)
      self.adc1 = self.data[0::4]
      self.adc2 = self.data[1::4]
      self.dac1 = self.data[2::4]
      getattr(self, self.mode)[0:self.sweep_size] = self.adc1[1:self.sweep_size + 1] / self.dac1[1:self.sweep_size + 1]
      getattr(self, 'plot_%s' % self.mode)()
      self.reading = False
      self.sweepFrame.setEnabled(True)
      self.selectFrame.setEnabled(True)

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

  def set_rate(self, value):
    if self.idle: return
    rate = [1, 10, 100, 1000][value]
    self.socket.write(struct.pack('<I', 3<<28 | int(rate)))

  def set_corr(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 4<<28 | int(value)))

  def sweep(self):
    if self.idle: return
    self.sweepFrame.setEnabled(False)
    self.selectFrame.setEnabled(False)
    self.socket.write(struct.pack('<I', 5<<28))
    self.offset = 0
    self.reading = True
    self.progress = QProgressDialog('Sweep status', 'Cancel', 0, self.sweep_size + 1)
    self.progress.setModal(True)
    self.progress.setMinimumDuration(1000)
    self.progress.canceled.connect(self.cancel)

  def cancel(self):
    self.offset = 0
    self.reading = False
    self.socket.write(struct.pack('<I', 6<<28))
    self.sweepFrame.setEnabled(True)
    self.selectFrame.setEnabled(True)

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
    size = self.sweep_size
    return 50.0 * (self.open[0:size] - self.load[0:size]) * (self.dut[0:size] - self.short[0:size]) / ((self.load[0:size] - self.short[0:size]) * (self.open[0:size] - self.dut[0:size]))

  def gamma(self):
    z = self.impedance()
    return (z - 50.0)/(z + 50.0)

  def plot_magphase(self, data):
    if self.cursor is not None: self.cursor.hide().disable()
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(left = 0.12, bottom = 0.12, right = 0.88, top = 0.98)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(FuncFormatter(metric_prefix))
    axes1.yaxis.set_major_formatter(FuncFormatter(metric_prefix))
    axes1.tick_params('y', color = 'blue', labelcolor = 'blue')
    axes1.yaxis.label.set_color('blue')
    axes1.plot(self.xaxis, np.absolute(data), color = 'blue', label = 'Magnitude')
    axes2 = axes1.twinx()
    axes2.spines['left'].set_color('blue')
    axes2.spines['right'].set_color('red')
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Magnitude')
    axes2.set_ylabel('Phase angle')
    axes2.tick_params('y', color = 'red', labelcolor = 'red')
    axes2.yaxis.label.set_color('red')
    axes2.plot(self.xaxis, np.angle(data, deg = True), color = 'red', label = 'Phase angle')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = '{label}: {y:.3e}\nFrequency: {x:.3e}'.format, display = 'multiple')
    self.canvas.draw()

  def plot_open(self):
    self.plot_magphase(self.open[0:self.sweep_size])

  def plot_short(self):
    self.plot_magphase(self.short[0:self.sweep_size])

  def plot_load(self):
    self.plot_magphase(self.load[0:self.sweep_size])

  def plot_dut(self):
    self.plot_magphase(self.dut[0:self.sweep_size])

  def plot_smith_grid(self, axes, color):
    load = 50.0
    ticks = np.array([0.0, 0.2, 0.5, 1.0, 2.0, 5.0])
    for tick in ticks * load:
      axis = np.logspace(-4, np.log10(1.0e3), 200) * load
      z = tick + 1.0j * axis
      gamma = (z - load)/(z + load)
      axes.plot(gamma.real, gamma.imag, color = color, linewidth = 0.4, alpha = 0.3)
      axes.plot(gamma.real, -gamma.imag, color = color, linewidth = 0.4, alpha = 0.3)
      z = axis + 1.0j * tick
      gamma = (z - load)/(z + load)
      axes.plot(gamma.real, gamma.imag, color = color, linewidth = 0.4, alpha = 0.3)
      axes.plot(gamma.real, -gamma.imag, color = color, linewidth = 0.4, alpha = 0.3)
      if tick == 0.0:
        axes.text(1.0, 0.0, u'\u221E', color = color, ha = 'left', va = 'center', clip_on = True, fontsize = 18.0)
        axes.text(-1.0, 0.0, u'0\u03A9', color = color, ha = 'left', va = 'bottom', clip_on = True, fontsize = 12.0)
        continue
      lab = u'%d\u03A9' % tick
      x = (tick - load) / (tick + load)
      axes.text(x, 0.0, lab, color = color, ha = 'left', va = 'bottom', clip_on = True, fontsize = 12.0)
      lab = u'j%d\u03A9' % tick
      z =  1.0j * tick
      gamma = (z - load)/(z + load) * 1.05
      x = gamma.real
      y = gamma.imag
      angle = np.angle(gamma) * 180.0 / np.pi - 90.0
      axes.text(x, y, lab, color = color, ha = 'center', va = 'center', clip_on = True, rotation = angle, fontsize = 12.0)
      lab = u'-j%d\u03A9' % tick
      axes.text(x, -y, lab, color = color, ha = 'center', va = 'center', clip_on = True, rotation = -angle, fontsize = 12.0)

  def plot_smith(self):
    if self.cursor is not None: self.cursor.hide().disable()
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(left = 0.01, bottom = 0.01, right = 0.99, top = 0.99)
    axes1 = self.figure.add_subplot(111)
    self.plot_smith_grid(axes1, 'blue')
    gamma = self.gamma()
    plot, = axes1.plot(gamma.real, gamma.imag, color = 'red')
    axes1.axis('equal')
    axes1.set_xlim(-1.12, 1.12)
    axes1.set_ylim(-1.12, 1.12)
    axes1.xaxis.set_visible(False)
    axes1.yaxis.set_visible(False)
    formatter = lambda x, y: '%1.1fM' % (x * 1e-6)
    self.cursor = datacursor(plot, formatter = GammaFormatter(self.xaxis), display = 'multiple')
    self.canvas.draw()

  def plot_imp(self):
    self.plot_magphase(self.impedance())

  def plot_rc(self):
    self.plot_magphase(self.gamma())

  def plot_swr(self):
    if self.cursor is not None: self.cursor.hide().disable()
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(left = 0.12, bottom = 0.12, right = 0.88, top = 0.98)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(FuncFormatter(metric_prefix))
    axes1.yaxis.set_major_formatter(FuncFormatter(metric_prefix))
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('SWR')
    magnitude = np.absolute(self.gamma())
    swr = np.maximum(1.0, np.minimum(100.0, (1.0 + magnitude) / np.maximum(1.0e-20, 1.0 - magnitude)))
    axes1.plot(self.xaxis, swr, color = 'blue', label = 'SWR')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = '{label}: {y:.3e}\nFrequency: {x:.3e}'.format, display = 'multiple')
    self.canvas.draw()

  def plot_rl(self):
    if self.cursor is not None: self.cursor.hide().disable()
    matplotlib.rcdefaults()
    self.figure.clf()
    self.figure.subplots_adjust(left = 0.12, bottom = 0.12, right = 0.88, top = 0.98)
    axes1 = self.figure.add_subplot(111)
    axes1.cla()
    axes1.xaxis.set_major_formatter(FuncFormatter(metric_prefix))
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Return loss, dB')
    magnitude = np.absolute(self.gamma())
    axes1.plot(self.xaxis, 20.0 * np.log10(magnitude), color = 'blue', label = 'Return loss')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = '{label}: {y:.3e}\nFrequency: {x:.3e}'.format, display = 'multiple')
    self.canvas.draw()

  def write_cfg(self):
    dialog = QFileDialog(self, 'Write configuration settings', '.', '*.ini')
    dialog.setDefaultSuffix('ini')
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    dialog.setOptions(QFileDialog.DontConfirmOverwrite)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      settings = QSettings(name[0], QSettings.IniFormat)
      self.write_cfg_settings(settings)

  def read_cfg(self):
    dialog = QFileDialog(self, 'Read configuration settings', '.', '*.ini')
    dialog.setDefaultSuffix('ini')
    dialog.setAcceptMode(QFileDialog.AcceptOpen)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      settings = QSettings(name[0], QSettings.IniFormat)
      self.read_cfg_settings(settings)

  def write_cfg_settings(self, settings):
    settings.setValue('addr', self.addrValue.text())
    settings.setValue('start', self.startValue.value())
    settings.setValue('stop', self.stopValue.value())
    settings.setValue('rate', self.rateValue.currentIndex())
    settings.setValue('corr', self.corrValue.value())
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
    self.addrValue.setText(settings.value('addr', '192.168.1.100'))
    self.startValue.setValue(settings.value('start', 100, type = int))
    self.stopValue.setValue(settings.value('stop', 60000, type = int))
    self.rateValue.setCurrentIndex(settings.value('rate', 0, type = int))
    self.corrValue.setValue(settings.value('corr', 0, type = int))
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
    dialog = QFileDialog(self, 'Write s1p file', '.', '*.s1p')
    dialog.setDefaultSuffix('s1p')
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    dialog.setOptions(QFileDialog.DontConfirmOverwrite)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      fh = open(name[0], 'w')
      gamma = self.gamma()
      size = self.sizeValue.value()
      fh.write('# GHz S MA R 50\n')
      for i in range(0, size):
        fh.write('0.0%.8d   %8.6f %7.2f\n' % (self.xaxis[i], np.absolute(gamma[i]), np.angle(gamma[i], deg = True)))
      fh.close()

warnings.filterwarnings('ignore')
app = QApplication(sys.argv)
window = VNA()
window.show()
sys.exit(app.exec())
