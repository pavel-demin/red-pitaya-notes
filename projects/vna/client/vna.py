#!/usr/bin/env python3

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
  if x == 0.0:
    return '0'
  elif abs(x) >= 1.0e6:
    return '%gM' % (x * 1.0e-6)
  elif abs(x) >= 1.0e3:
    return '%gk' % (x * 1.0e-3)
  elif abs(x) >= 1.0e0:
    return '%g' % x
  elif abs(x) >= 1.0e-3:
    return '%gm' % (x * 1e+3)
  elif abs(x) >= 1.0e-6:
    return '%gu' % (x * 1e+6)
  else:
    return '%g' % x

class SmithFormatter:
  def __init__(self, freq):
    self.freq = freq
  def __call__(self, x = None, y = None, ind = None, **kwargs):
    gamma = complex(x, y)
    z = 50.0 * (1.0 + gamma)/(1.0 - gamma)
    if z.imag >= 0.0:
      return u'Z: %s\u03A9 + j%s\u03A9\nFrequency: %sHz' % (metric_prefix(z.real), metric_prefix(z.imag), metric_prefix(self.freq[ind[0]]))
    else:
      return u'Z: %s\u03A9 \u2212 j%s\u03A9\nFrequency: %sHz' % (metric_prefix(z.real), metric_prefix(-z.imag), metric_prefix(self.freq[ind[0]]))

class LabelFormatter:
  def __call__(self, x = None, y = None, label = None, **kwargs):
    return '%s: %s\nFrequency: %sHz'  % (label, metric_prefix(y), metric_prefix(x))

class Measurement:
  def __init__(self, start, stop, size):
    self.freq = np.linspace(start, stop, size) * 1000
    self.data = np.zeros(size, np.complex64)

class VNA(QMainWindow, Ui_VNA):

  max_size = 32768

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
    self.sweep_start = 10
    self.sweep_stop = 60000
    self.sweep_size = 6000
    # buffer and offset for the incoming samples
    self.buffer = bytearray(16 * VNA.max_size)
    self.offset = 0
    self.data = np.frombuffer(self.buffer, np.complex64)
    self.open = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
    self.short = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
    self.load = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
    self.dut = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
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
    if int(matplotlib.__version__[0]) < 2:
      self.toolbar.removeAction(actions[7])
    else:
      self.toolbar.removeAction(actions[6])
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
    self.csvButton.clicked.connect(self.write_csv)
    self.s1pButton.clicked.connect(self.write_s1p)
    self.s2pButton.clicked.connect(self.write_s2p)
    self.startValue.valueChanged.connect(self.set_start)
    self.stopValue.valueChanged.connect(self.set_stop)
    self.sizeValue.valueChanged.connect(self.set_size)
    self.rateValue.addItems(['10000', '5000', '1000', '500', '100', '50', '10', '5', '1'])
    self.rateValue.lineEdit().setReadOnly(True)
    self.rateValue.lineEdit().setAlignment(Qt.AlignRight)
    for i in range(0, self.rateValue.count()):
      self.rateValue.setItemData(i, Qt.AlignRight, Qt.TextAlignmentRole)
    self.rateValue.currentIndexChanged.connect(self.set_rate)
    self.corrValue.valueChanged.connect(self.set_corr)
    self.levelValue.valueChanged.connect(self.set_level)
    self.openPlot.clicked.connect(self.plot_open)
    self.shortPlot.clicked.connect(self.plot_short)
    self.loadPlot.clicked.connect(self.plot_load)
    self.dutPlot.clicked.connect(self.plot_dut)
    self.smithPlot.clicked.connect(self.plot_smith)
    self.impPlot.clicked.connect(self.plot_imp)
    self.rcPlot.clicked.connect(self.plot_rc)
    self.swrPlot.clicked.connect(self.plot_swr)
    self.rlPlot.clicked.connect(self.plot_rl)
    self.gainPlot.clicked.connect(self.plot_gain)
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
    self.set_level(self.levelValue.value())
    self.connectButton.setText('Disconnect')
    self.connectButton.setEnabled(True)
    self.sweepFrame.setEnabled(True)
    self.dutSweep.setEnabled(True)

  def read_data(self):
    while(self.socket.bytesAvailable() > 0):
      if not self.reading:
        self.socket.readAll()
        return
      size = self.socket.bytesAvailable()
      self.progress.setValue((self.offset + size) / 16)
      limit = 16 * self.sweep_size
      if self.offset + size < limit:
        self.buffer[self.offset:self.offset + size] = self.socket.read(size)
        self.offset += size
      else:
        self.buffer[self.offset:limit] = self.socket.read(limit - self.offset)
        adc1 = self.data[0::2]
        adc2 = self.data[1::2]
        attr = getattr(self, self.mode)
        size = attr.freq.size
        attr.data = adc1[0:size].copy()
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

  def set_stop(self, value):
    self.sweep_stop = value

  def set_size(self, value):
    self.sweep_size = value

  def set_rate(self, value):
    if self.idle: return
    rate = [5, 10, 50, 100, 500, 1000, 5000, 10000, 50000][value]
    self.socket.write(struct.pack('<I', 3<<28 | int(rate)))

  def set_corr(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 4<<28 | int(value)))

  def set_level(self, value):
    if self.idle: return
    self.socket.write(struct.pack('<I', 5<<28 | int(32766 * np.power(10.0, value / 20.0))))
    self.socket.write(struct.pack('<I', 6<<28 | int(0)))

  def sweep(self, mode):
    if self.idle: return
    self.sweepFrame.setEnabled(False)
    self.selectFrame.setEnabled(False)
    self.mode = mode
    attr = getattr(self, mode)
    attr.freq = np.linspace(self.sweep_start, self.sweep_stop, self.sweep_size) * 1000
    self.offset = 0
    self.reading = True
    self.socket.write(struct.pack('<I', 0<<28 | int(self.sweep_start * 1000)))
    self.socket.write(struct.pack('<I', 1<<28 | int(self.sweep_stop * 1000)))
    self.socket.write(struct.pack('<I', 2<<28 | int(self.sweep_size)))
    self.socket.write(struct.pack('<I', 7<<28))
    self.progress = QProgressDialog('Sweep status', 'Cancel', 0, self.sweep_size)
    self.progress.setModal(True)
    self.progress.setMinimumDuration(1000)
    self.progress.canceled.connect(self.cancel)

  def cancel(self):
    self.offset = 0
    self.reading = False
    self.socket.write(struct.pack('<I', 8<<28))
    self.sweepFrame.setEnabled(True)
    self.selectFrame.setEnabled(True)

  def sweep_open(self):
    self.sweep('open')

  def sweep_short(self):
    self.sweep('short')

  def sweep_load(self):
    self.sweep('load')

  def sweep_dut(self):
    self.sweep('dut')

  def interp(self, freq, data):
    real = np.interp(self.dut.freq, freq, data.real)
    imag = np.interp(self.dut.freq, freq, data.imag)
    return real + 1j * imag

  def gain(self):
    return self.dut.data/self.interp(self.short.freq, self.short.data)

  def impedance(self):
    open = self.interp(self.open.freq, self.open.data)
    short = self.interp(self.short.freq, self.short.data)
    load = self.interp(self.load.freq, self.load.data)
    dut = self.dut.data
    return 50.0 * (open - load) * (dut - short) / ((load - short) * (open - dut))

  def gamma(self):
    z = self.impedance()
    return (z - 50.0)/(z + 50.0)

  def plot_gain(self):
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
    gain = self.gain()
    axes1.plot(self.dut.freq, 20.0 * np.log10(np.absolute(gain)), color = 'blue', label = 'Gain')
    axes2 = axes1.twinx()
    axes2.spines['left'].set_color('blue')
    axes2.spines['right'].set_color('red')
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Gain, dB')
    axes2.set_ylabel('Phase angle')
    axes2.tick_params('y', color = 'red', labelcolor = 'red')
    axes2.yaxis.label.set_color('red')
    axes2.plot(self.dut.freq, np.angle(gain, deg = True), color = 'red', label = 'Phase angle')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = LabelFormatter(), display = 'multiple')
    self.canvas.draw()

  def plot_magphase(self, freq, data):
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
    axes1.plot(freq, np.absolute(data), color = 'blue', label = 'Magnitude')
    axes2 = axes1.twinx()
    axes2.spines['left'].set_color('blue')
    axes2.spines['right'].set_color('red')
    axes1.set_xlabel('Hz')
    axes1.set_ylabel('Magnitude')
    axes2.set_ylabel('Phase angle')
    axes2.tick_params('y', color = 'red', labelcolor = 'red')
    axes2.yaxis.label.set_color('red')
    axes2.plot(freq, np.angle(data, deg = True), color = 'red', label = 'Phase angle')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = LabelFormatter(), display = 'multiple')
    self.canvas.draw()

  def plot_open(self):
    self.plot_magphase(self.open.freq, self.open.data)

  def plot_short(self):
    self.plot_magphase(self.short.freq, self.short.data)

  def plot_load(self):
    self.plot_magphase(self.load.freq, self.load.data)

  def plot_dut(self):
    self.plot_magphase(self.dut.freq, self.dut.data)

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
    self.figure.subplots_adjust(left = 0.0, bottom = 0.0, right = 1.0, top = 1.0)
    axes1 = self.figure.add_subplot(111)
    self.plot_smith_grid(axes1, 'blue')
    gamma = self.gamma()
    plot, = axes1.plot(gamma.real, gamma.imag, color = 'red')
    axes1.axis('equal')
    axes1.set_xlim(-1.12, 1.12)
    axes1.set_ylim(-1.12, 1.12)
    axes1.xaxis.set_visible(False)
    axes1.yaxis.set_visible(False)
    for loc, spine in axes1.spines.items():
      spine.set_visible(False)
    self.cursor = datacursor(plot, formatter = SmithFormatter(self.dut.freq), display = 'multiple')
    self.canvas.draw()

  def plot_imp(self):
    self.plot_magphase(self.dut.freq, self.impedance())

  def plot_rc(self):
    self.plot_magphase(self.dut.freq, self.gamma())

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
    axes1.plot(self.dut.freq, swr, color = 'blue', label = 'SWR')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = LabelFormatter(), display = 'multiple')
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
    axes1.plot(self.dut.freq, 20.0 * np.log10(magnitude), color = 'blue', label = 'Return loss')
    self.cursor = datacursor(axes = self.figure.get_axes(), formatter = LabelFormatter(), display = 'multiple')
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
    settings.setValue('rate', self.rateValue.currentIndex())
    settings.setValue('corr', self.corrValue.value())
    settings.setValue('open_start', int(self.open.freq[0]))
    settings.setValue('open_stop', int(self.open.freq[-1]))
    settings.setValue('open_size', self.open.freq.size)
    settings.setValue('short_start', int(self.short.freq[0]))
    settings.setValue('short_stop', int(self.short.freq[-1]))
    settings.setValue('short_size', self.short.freq.size)
    settings.setValue('load_start', int(self.load.freq[0]))
    settings.setValue('load_stop', int(self.load.freq[-1]))
    settings.setValue('load_size', self.load.freq.size)
    settings.setValue('dut_start', int(self.dut.freq[0]))
    settings.setValue('dut_stop', int(self.dut.freq[-1]))
    settings.setValue('dut_size', self.dut.freq.size)
    data = self.open.data
    for i in range(0, self.open.freq.size):
      settings.setValue('open_real_%d' % i, float(data.real[i]))
      settings.setValue('open_imag_%d' % i, float(data.imag[i]))
    data = self.short.data
    for i in range(0, self.short.freq.size):
      settings.setValue('short_real_%d' % i, float(data.real[i]))
      settings.setValue('short_imag_%d' % i, float(data.imag[i]))
    data = self.load.data
    for i in range(0, self.load.freq.size):
      settings.setValue('load_real_%d' % i, float(data.real[i]))
      settings.setValue('load_imag_%d' % i, float(data.imag[i]))
    data = self.dut.data
    for i in range(0, self.dut.freq.size):
      settings.setValue('dut_real_%d' % i, float(data.real[i]))
      settings.setValue('dut_imag_%d' % i, float(data.imag[i]))

  def read_cfg_settings(self, settings):
    self.addrValue.setText(settings.value('addr', '192.168.1.100'))
    self.rateValue.setCurrentIndex(settings.value('rate', 0, type = int))
    self.corrValue.setValue(settings.value('corr', 0, type = int))
    open_start = settings.value('open_start', 10000, type = int) // 1000
    open_stop = settings.value('open_stop', 60000000, type = int) // 1000
    open_size = settings.value('open_size', 6000, type = int)
    short_start = settings.value('short_start', 10000, type = int) // 1000
    short_stop = settings.value('short_stop', 60000000, type = int) // 1000
    short_size = settings.value('short_size', 6000, type = int)
    load_start = settings.value('load_start', 10000, type = int) // 1000
    load_stop = settings.value('load_stop', 60000000, type = int) // 1000
    load_size = settings.value('load_size', 6000, type = int)
    dut_start = settings.value('dut_start', 10000, type = int) // 1000
    dut_stop = settings.value('dut_stop', 60000000, type = int) // 1000
    dut_size = settings.value('dut_size', 6000, type = int)
    self.startValue.setValue(dut_start)
    self.stopValue.setValue(dut_stop)
    self.sizeValue.setValue(dut_size)
    self.open = Measurement(open_start, open_stop, open_size)
    for i in range(0, open_size):
      real = settings.value('open_real_%d' % i, 0.0, type = float)
      imag = settings.value('open_imag_%d' % i, 0.0, type = float)
      self.open.data[i] = real + 1.0j * imag
    self.short = Measurement(short_start, short_stop, short_size)
    for i in range(0, short_size):
      real = settings.value('short_real_%d' % i, 0.0, type = float)
      imag = settings.value('short_imag_%d' % i, 0.0, type = float)
      self.short.data[i] = real + 1.0j * imag
    self.load = Measurement(load_start, load_stop, load_size)
    for i in range(0, load_size):
      real = settings.value('load_real_%d' % i, 0.0, type = float)
      imag = settings.value('load_imag_%d' % i, 0.0, type = float)
      self.load.data[i] = real + 1.0j * imag
    self.dut = Measurement(dut_start, dut_stop, dut_size)
    for i in range(0, dut_size):
      real = settings.value('dut_real_%d' % i, 0.0, type = float)
      imag = settings.value('dut_imag_%d' % i, 0.0, type = float)
      self.dut.data[i] = real + 1.0j * imag

  def write_csv(self):
    dialog = QFileDialog(self, 'Write csv file', '.', '*.csv')
    dialog.setDefaultSuffix('csv')
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    dialog.setOptions(QFileDialog.DontConfirmOverwrite)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      fh = open(name[0], 'w')
      o = self.interp(self.open.freq, self.open.data)
      s = self.interp(self.short.freq, self.short.data)
      l = self.interp(self.load.freq, self.load.data)
      d = self.dut.data
      f = self.dut.freq
      fh.write('frequency;open.real;open.imag;short.real;short.imag;load.real;load.imag;dut.real;dut.imag\n')
      for i in range(0, f.size):
        fh.write('0.0%.8d;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f\n' % (f[i], o.real[i], o.imag[i], s.real[i], s.imag[i], l.real[i], l.imag[i], d.real[i], d.imag[i]))
      fh.close()

  def write_s1p(self):
    dialog = QFileDialog(self, 'Write s1p file', '.', '*.s1p')
    dialog.setDefaultSuffix('s1p')
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    dialog.setOptions(QFileDialog.DontConfirmOverwrite)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      fh = open(name[0], 'w')
      gamma = self.gamma()
      freq = self.dut.freq
      fh.write('# GHz S MA R 50\n')
      for i in range(0, freq.size):
        fh.write('0.0%.8d   %8.6f %7.2f\n' % (freq[i], np.absolute(gamma[i]), np.angle(gamma[i], deg = True)))
      fh.close()

  def write_s2p(self):
    dialog = QFileDialog(self, 'Write s2p file', '.', '*.s2p')
    dialog.setDefaultSuffix('s2p')
    dialog.setAcceptMode(QFileDialog.AcceptSave)
    dialog.setOptions(QFileDialog.DontConfirmOverwrite)
    if dialog.exec() == QDialog.Accepted:
      name = dialog.selectedFiles()
      fh = open(name[0], 'w')
      gain = self.gain()
      gamma = self.gamma()
      freq = self.dut.freq
      fh.write('# GHz S MA R 50\n')
      for i in range(0, freq.size):
        fh.write('0.0%.8d   %8.6f %7.2f   %8.6f %7.2f   0.000000    0.00   0.000000    0.00\n' % (freq[i], np.absolute(gamma[i]), np.angle(gamma[i], deg = True), np.absolute(gain[i]), np.angle(gain[i], deg = True)))
      fh.close()

warnings.filterwarnings('ignore')
app = QApplication(sys.argv)
window = VNA()
window.show()
sys.exit(app.exec())
