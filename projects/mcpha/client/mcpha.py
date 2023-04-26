#!/usr/bin/env python3

import sys
import time
import struct

from functools import partial

import numpy as np

from matplotlib.figure import Figure

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

from PySide2.QtUiTools import loadUiType
from PySide2.QtCore import Qt, QTimer, QEventLoop, QRegExp
from PySide2.QtGui import QPalette, QColor, QRegExpValidator
from PySide2.QtWidgets import QApplication, QMainWindow, QDialog, QFileDialog
from PySide2.QtWidgets import QWidget, QLabel, QCheckBox, QComboBox
from PySide2.QtNetwork import QAbstractSocket, QTcpSocket

Ui_MCPHA, QMainWindow = loadUiType("mcpha.ui")
Ui_LogDisplay, QWidget = loadUiType("mcpha_log.ui")
Ui_HstDisplay, QWidget = loadUiType("mcpha_hst.ui")
Ui_OscDisplay, QWidget = loadUiType("mcpha_osc.ui")
Ui_GenDisplay, QWidget = loadUiType("mcpha_gen.ui")


class MCPHA(QMainWindow, Ui_MCPHA):
    def __init__(self):
        super(MCPHA, self).__init__()
        self.setupUi(self)
        # initialize variables
        self.idle = True
        self.waiting = [False for i in range(3)]
        self.reset = 0
        self.state = 0
        self.status = np.zeros(9, np.uint32)
        self.timers = self.status[:4].view(np.uint64)
        # create tabs
        self.log = LogDisplay()
        self.hst1 = HstDisplay(self, self.log, 0)
        self.hst2 = HstDisplay(self, self.log, 1)
        self.osc = OscDisplay(self, self.log)
        self.gen = GenDisplay(self, self.log)
        self.tabWidget.addTab(self.log, "Messages")
        self.tabWidget.addTab(self.hst1, "Spectrum histogram 1")
        self.tabWidget.addTab(self.hst2, "Spectrum histogram 2")
        self.tabWidget.addTab(self.osc, "Oscilloscope")
        self.tabWidget.addTab(self.gen, "Pulse generator")
        # configure controls
        self.connectButton.clicked.connect(self.start)
        self.neg1Check.toggled.connect(partial(self.set_negator, 0))
        self.neg2Check.toggled.connect(partial(self.set_negator, 1))
        self.rateValue.valueChanged.connect(self.set_rate)
        # address validator
        rx = QRegExp("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])|rp-[0-9A-Fa-f]{6}\.local$")
        self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
        # create TCP socket
        self.socket = QTcpSocket(self)
        self.socket.connected.connect(self.connected)
        self.socket.error.connect(self.display_error)
        # create event loop
        self.loop = QEventLoop()
        self.socket.readyRead.connect(self.loop.quit)
        self.socket.error.connect(self.loop.quit)
        # create timers
        self.startTimer = QTimer(self)
        self.startTimer.timeout.connect(self.start_timeout)
        self.readTimer = QTimer(self)
        self.readTimer.timeout.connect(self.read_timeout)

    def start(self):
        self.socket.connectToHost(self.addrValue.text(), 1001)
        self.startTimer.start(5000)
        self.connectButton.setText("Disconnect")
        self.connectButton.clicked.disconnect()
        self.connectButton.clicked.connect(self.stop)

    def stop(self):
        self.hst1.stop()
        self.hst2.stop()
        self.osc.stop()
        self.readTimer.stop()
        self.startTimer.stop()
        self.loop.quit()
        self.socket.abort()
        self.connectButton.setText("Connect")
        self.connectButton.clicked.disconnect()
        self.connectButton.clicked.connect(self.start)
        self.log.print("IO stopped")
        self.idle = True

    def start_timeout(self):
        self.log.print("error: connection timeout")
        self.stop()

    def display_error(self):
        self.log.print("error: %s" % self.socket.errorString())
        self.stop()

    def connected(self):
        self.startTimer.stop()
        self.readTimer.start(500)
        self.log.print("IO started")
        self.idle = False
        self.waiting = [False for i in range(3)]
        self.reset = 0
        self.state = 0
        self.set_rate(self.rateValue.value())
        self.set_negator(0, self.neg1Check.isChecked())
        self.set_negator(1, self.neg2Check.isChecked())

    def command(self, code, number, value):
        self.socket.write(struct.pack("<Q", code << 56 | number << 52 | (int(value) & 0xFFFFFFFFFFFFF)))

    def read_data(self, data):
        view = data.view(np.uint8)
        size = view.size
        while self.socket.state() == QAbstractSocket.ConnectedState and self.socket.bytesAvailable() < size:
            self.loop.exec_()
        if self.socket.bytesAvailable() < size:
            return False
        else:
            view[:] = self.socket.read(size)
            return True

    def read_timeout(self):
        # send reset commands
        if self.reset & 1:
            self.command(0, 0, 0)
        if self.reset & 2:
            self.command(0, 1, 0)
        if self.reset & 4:
            self.command(1, 0, 0)
        if self.reset & 8:
            self.command(1, 1, 0)
        if self.reset & 16:
            self.command(2, 0, 0)
        if self.reset & 32:
            self.command(19, 0, 0)
        self.reset = 0
        # read
        self.command(11, 0, 0)
        if not self.read_data(self.status):
            return
        if self.waiting[0]:
            self.command(12, 0, 0)
            if self.read_data(self.hst1.buffer):
                self.hst1.update(self.timers[0])
            else:
                return
        if self.waiting[1]:
            self.command(12, 1, 0)
            if self.read_data(self.hst2.buffer):
                self.hst2.update(self.timers[1])
            else:
                return
        if self.waiting[2] and not self.status[8] & 1:
            self.command(20, 0, 0)
            if self.read_data(self.osc.buffer):
                self.osc.update()
                self.reset_osc()
                self.start_osc()
            else:
                return

    def reset_hst(self, number):
        if number == 0:
            self.reset |= 1
        else:
            self.reset |= 2

    def reset_timer(self, number):
        if number == 0:
            self.reset |= 4
        else:
            self.reset |= 8

    def reset_osc(self):
        self.reset |= 16

    def start_osc(self):
        self.reset |= 32
        self.waiting[2] = True

    def stop_osc(self):
        self.reset &= ~32
        self.waiting[2] = False

    def set_rate(self, value):
        self.command(4, 0, value)

    def set_negator(self, number, value):
        self.command(5, number, value)

    def set_pha_delay(self, number, value):
        self.command(6, number, value)

    def set_pha_thresholds(self, number, min, max):
        self.command(7, number, min)
        self.command(8, number, max)

    def set_timer(self, number, value):
        self.command(9, number, value)

    def set_timer_mode(self, number, value):
        self.command(10, number, value)
        self.waiting[number] = value

    def set_trg_source(self, number):
        self.command(13, number, 0)

    def set_trg_slope(self, value):
        self.command(14, 0, value)

    def set_trg_mode(self, value):
        self.command(15, 0, value)

    def set_trg_level(self, value):
        self.command(16, 0, value)

    def set_osc_pre(self, value):
        self.command(17, 0, value)

    def set_osc_tot(self, value):
        self.command(18, 0, value)

    def set_gen_fall(self, value):
        self.command(21, 0, value)

    def set_gen_rise(self, value):
        self.command(22, 0, value)

    def set_gen_rate(self, value):
        self.command(25, 0, value)

    def set_gen_dist(self, value):
        self.command(26, 0, value)

    def reset_gen(self):
        self.command(27, 0)

    def set_gen_bin(self, value):
        self.command(28, 0, value)

    def start_gen(self):
        self.command(29, 0, 1)

    def stop_gen(self):
        self.command(30, 0, 0)


class LogDisplay(QWidget, Ui_LogDisplay):
    def __init__(self):
        super(LogDisplay, self).__init__()
        self.setupUi(self)

    def print(self, text):
        self.logViewer.appendPlainText(text)


class HstDisplay(QWidget, Ui_HstDisplay):
    def __init__(self, mcpha, log, number):
        super(HstDisplay, self).__init__()
        self.setupUi(self)
        # initialize variables
        self.mcpha = mcpha
        self.log = log
        self.number = number
        self.min = 0
        self.max = 16383
        self.sum = 0
        self.time = np.uint64([75e8, 0])
        self.factor = 1
        self.bins = 16384
        self.buffer = np.zeros(self.bins, np.uint32)
        if number == 0:
            self.color = "#FFAA00"
        else:
            self.color = "#00CCCC"
        # create figure
        self.figure = Figure()
        if sys.platform != "win32":
            self.figure.set_facecolor("none")
        self.figure.subplots_adjust(left=0.18, bottom=0.08, right=0.98, top=0.95)
        self.canvas = FigureCanvas(self.figure)
        self.plotLayout.addWidget(self.canvas)
        self.ax = self.figure.add_subplot(111)
        self.ax.grid()
        x = np.arange(self.bins)
        (self.curve,) = self.ax.plot(x, self.buffer, drawstyle="steps-mid", color=self.color)
        # create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, None, False)
        self.toolbar.layout().setSpacing(6)
        # remove subplots action
        actions = self.toolbar.actions()
        self.toolbar.removeAction(actions[6])
        self.toolbar.removeAction(actions[7])
        self.logCheck = QCheckBox("log scale")
        self.logCheck.setChecked(True)
        self.binsLabel = QLabel("rebin factor")
        self.binsValue = QComboBox()
        self.binsValue.addItems(["1", "2", "4", "8", "16"])
        self.binsValue.setEditable(True)
        self.binsValue.lineEdit().setReadOnly(True)
        self.binsValue.lineEdit().setAlignment(Qt.AlignRight)
        for i in range(self.binsValue.count()):
            self.binsValue.setItemData(i, Qt.AlignRight, Qt.TextAlignmentRole)
        self.toolbar.addSeparator()
        self.toolbar.addWidget(self.logCheck)
        self.toolbar.addSeparator()
        self.toolbar.addWidget(self.binsLabel)
        self.toolbar.addWidget(self.binsValue)
        self.plotLayout.addWidget(self.toolbar)
        # configure controls
        actions[0].triggered.disconnect()
        actions[0].triggered.connect(self.home)
        self.logCheck.toggled.connect(self.set_scale)
        self.binsValue.currentIndexChanged.connect(self.set_bins)
        self.thrsCheck.toggled.connect(self.set_thresholds)
        self.startButton.clicked.connect(self.start)
        self.saveButton.clicked.connect(self.save)
        self.loadButton.clicked.connect(self.load)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)
        # update controls
        self.set_thresholds(self.thrsCheck.isChecked())
        self.set_time(self.time[0])
        self.set_scale(self.logCheck.isChecked())

    def start(self):
        if self.mcpha.idle:
            return
        self.set_thresholds(self.thrsCheck.isChecked())
        self.set_enable(False)
        h = self.hoursValue.value()
        m = self.minutesValue.value()
        s = self.secondsValue.value()
        value = (h * 3600000 + m * 60000 + s * 1000) * 125000
        self.sum = 0
        self.time[:] = [value, 0]
        self.mcpha.reset_hst(self.number)
        self.mcpha.reset_timer(self.number)
        self.mcpha.set_pha_delay(self.number, 100)
        self.mcpha.set_pha_thresholds(self.number, self.min, self.max)
        self.mcpha.set_timer(self.number, value)
        self.mcpha.set_timer_mode(self.number, 1)
        self.startButton.setText("Stop")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.stop)
        self.log.print("timer %d started" % (self.number + 1))

    def stop(self):
        self.mcpha.set_timer_mode(self.number, 0)
        self.set_enable(True)
        self.set_time(self.time[0])
        self.startButton.setText("Start")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.start)
        self.log.print("timer %d stopped" % (self.number + 1))

    def set_enable(self, value):
        if value:
            self.set_thresholds(self.thrsCheck.isChecked())
        else:
            self.minValue.setEnabled(False)
            self.maxValue.setEnabled(False)
        self.thrsCheck.setEnabled(value)
        self.hoursValue.setEnabled(value)
        self.minutesValue.setEnabled(value)
        self.secondsValue.setEnabled(value)

    def home(self):
        self.set_scale(self.logCheck.isChecked())

    def set_scale(self, checked):
        self.toolbar.home()
        self.toolbar.update()
        if checked:
            self.ax.set_ylim(1, 1e10)
            self.ax.set_yscale("log")
        else:
            self.ax.set_ylim(auto=True)
            self.ax.set_yscale("linear")
        size = self.bins // self.factor
        self.ax.set_xlim(-0.05 * size, size * 1.05)
        self.ax.relim()
        self.ax.autoscale_view(scalex=True, scaley=True)
        self.canvas.draw()

    def set_bins(self, value):
        factor = 1 << value
        self.factor = factor
        bins = self.bins // self.factor
        x = np.arange(bins)
        y = self.buffer.reshape(-1, self.factor).sum(-1)
        self.curve.set_xdata(x)
        self.curve.set_ydata(y)
        self.set_scale(self.logCheck.isChecked())

    def set_thresholds(self, checked):
        self.minValue.setEnabled(checked)
        self.maxValue.setEnabled(checked)
        if checked:
            self.min = self.minValue.value()
            self.max = self.maxValue.value()
        else:
            self.min = 0
            self.max = 16383

    def set_time(self, value):
        value = value // 125000
        h, mod = divmod(value, 3600000)
        m, mod = divmod(mod, 60000)
        s = mod / 1000
        self.hoursValue.setValue(int(h))
        self.minutesValue.setValue(int(m))
        self.secondsValue.setValue(s)

    def update(self, value):
        self.update_rate(value)
        self.update_time(value)
        self.update_plot()

    def update_rate(self, value):
        sum = self.buffer.sum()
        self.totalValue.setText("%.2e" % sum)
        if value > self.time[1]:
            rate = (sum - self.sum) / (value - self.time[1]) * 125e6
            self.instValue.setText("%.2e" % rate)
        if value > 0:
            rate = sum / value * 125e6
            self.avgValue.setText("%.2e" % rate)
        self.sum = sum
        self.time[1] = value

    def update_time(self, value):
        if value < self.time[0]:
            self.set_time(self.time[0] - value)
        else:
            self.stop()

    def update_plot(self):
        y = self.buffer.reshape(-1, self.factor).sum(-1)
        self.curve.set_ydata(y)
        self.ax.relim()
        self.ax.autoscale_view(scalex=False, scaley=True)
        self.canvas.draw()

    def on_motion(self, event):
        if event.inaxes != self.ax:
            return
        x = int(event.xdata + 0.5)
        if x < 0 or x >= self.bins // self.factor:
            return
        y = self.curve.get_ydata(True)[x]
        self.numberValue.setText("%d" % x)
        self.entriesValue.setText("%d" % y)

    def save(self):
        try:
            dialog = QFileDialog(self, "Save hst file", ".", "*.hst")
            dialog.setDefaultSuffix("hst")
            name = "histogram-%s.hst" % time.strftime("%Y%m%d-%H%M%S")
            dialog.selectFile(name)
            dialog.setAcceptMode(QFileDialog.AcceptSave)
            if dialog.exec() == QDialog.Accepted:
                name = dialog.selectedFiles()
                self.buffer.tofile(name[0])
                self.log.print("histogram %d saved to file %s" % ((self.number + 1), name[0]))
        except:
            self.log.print("error: %s" % sys.exc_info()[1])

    def load(self):
        try:
            dialog = QFileDialog(self, "Load hst file", ".", "*.hst")
            dialog.setDefaultSuffix("hst")
            dialog.setAcceptMode(QFileDialog.AcceptOpen)
            if dialog.exec() == QDialog.Accepted:
                name = dialog.selectedFiles()
                self.buffer[:] = np.fromfile(name[0], np.uint32)
                self.update_plot()
        except:
            self.log.print("error: %s" % sys.exc_info()[1])


class OscDisplay(QWidget, Ui_OscDisplay):
    def __init__(self, mcpha, log):
        super(OscDisplay, self).__init__()
        self.setupUi(self)
        # initialize variables
        self.mcpha = mcpha
        self.log = log
        self.pre = 10000
        self.tot = 100000
        self.buffer = np.zeros(self.tot * 2, np.int16)
        # create figure
        self.figure = Figure()
        if sys.platform != "win32":
            self.figure.set_facecolor("none")
        self.figure.subplots_adjust(left=0.18, bottom=0.08, right=0.98, top=0.95)
        self.canvas = FigureCanvas(self.figure)
        self.plotLayout.addWidget(self.canvas)
        self.ax = self.figure.add_subplot(111)
        self.ax.grid()
        self.ax.set_ylim(-16500, 16500)
        x = np.arange(self.tot)
        (self.curve2,) = self.ax.plot(x, self.buffer[1::2], color="#00CCCC")
        (self.curve1,) = self.ax.plot(x, self.buffer[0::2], color="#FFAA00")
        self.canvas.draw()
        # create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, None, False)
        self.toolbar.layout().setSpacing(6)
        # remove subplots action
        actions = self.toolbar.actions()
        self.toolbar.removeAction(actions[6])
        self.toolbar.removeAction(actions[7])
        # configure colors
        self.plotLayout.addWidget(self.toolbar)
        palette = QPalette(self.ch1Label.palette())
        palette.setColor(QPalette.Window, QColor("#FFAA00"))
        palette.setColor(QPalette.WindowText, QColor("black"))
        self.ch1Label.setAutoFillBackground(True)
        self.ch1Label.setPalette(palette)
        self.ch1Value.setAutoFillBackground(True)
        self.ch1Value.setPalette(palette)
        palette.setColor(QPalette.Window, QColor("#00CCCC"))
        palette.setColor(QPalette.WindowText, QColor("black"))
        self.ch2Label.setAutoFillBackground(True)
        self.ch2Label.setPalette(palette)
        self.ch2Value.setAutoFillBackground(True)
        self.ch2Value.setPalette(palette)
        # configure controls
        self.autoButton.toggled.connect(self.mcpha.set_trg_mode)
        self.ch2Button.toggled.connect(self.mcpha.set_trg_source)
        self.fallingButton.toggled.connect(self.mcpha.set_trg_slope)
        self.levelValue.valueChanged.connect(self.mcpha.set_trg_level)
        self.startButton.clicked.connect(self.start)
        self.saveButton.clicked.connect(self.save)
        self.loadButton.clicked.connect(self.load)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)

    def start(self):
        if self.mcpha.idle:
            return
        self.mcpha.set_trg_mode(self.autoButton.isChecked())
        self.mcpha.set_trg_source(self.ch2Button.isChecked())
        self.mcpha.set_trg_slope(self.fallingButton.isChecked())
        self.mcpha.set_trg_level(self.levelValue.value())
        self.mcpha.set_osc_pre(self.pre)
        self.mcpha.set_osc_tot(self.tot)
        self.mcpha.reset_osc()
        self.mcpha.start_osc()
        self.startButton.setText("Stop")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.stop)
        self.log.print("oscilloscope started")

    def stop(self):
        self.mcpha.stop_osc()
        self.startButton.setText("Start")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.start)
        self.log.print("oscilloscope stopped")

    def update(self):
        self.curve1.set_ydata(self.buffer[0::2])
        self.curve2.set_ydata(self.buffer[1::2])
        self.canvas.draw()

    def on_motion(self, event):
        if event.inaxes != self.ax:
            return
        x = int(event.xdata + 0.5)
        if x < 0 or x >= self.tot:
            return
        y1 = self.curve1.get_ydata(True)[x]
        y2 = self.curve2.get_ydata(True)[x]
        self.timeValue.setText("%d" % x)
        self.ch1Value.setText("%d" % y1)
        self.ch2Value.setText("%d" % y2)

    def save(self):
        try:
            dialog = QFileDialog(self, "Save osc file", ".", "*.osc")
            dialog.setDefaultSuffix("osc")
            name = "oscillogram-%s.osc" % time.strftime("%Y%m%d-%H%M%S")
            dialog.selectFile(name)
            dialog.setAcceptMode(QFileDialog.AcceptSave)
            if dialog.exec() == QDialog.Accepted:
                name = dialog.selectedFiles()
                self.buffer.tofile(name[0])
                self.log.print("histogram %d saved to file %s" % ((self.number + 1), name[0]))
        except:
            self.log.print("error: %s" % sys.exc_info()[1])

    def load(self):
        try:
            dialog = QFileDialog(self, "Load osc file", ".", "*.osc")
            dialog.setDefaultSuffix("osc")
            dialog.setAcceptMode(QFileDialog.AcceptOpen)
            if dialog.exec() == QDialog.Accepted:
                name = dialog.selectedFiles()
                self.buffer[:] = np.fromfile(name[0], np.int16)
                self.update()
        except:
            self.log.print("error: %s" % sys.exc_info()[1])


class GenDisplay(QWidget, Ui_GenDisplay):
    def __init__(self, mcpha, log):
        super(GenDisplay, self).__init__()
        self.setupUi(self)
        # initialize variables
        self.mcpha = mcpha
        self.log = log
        self.bins = 4096
        self.buffer = np.zeros(self.bins, np.uint32)
        # create figure
        self.figure = Figure()
        if sys.platform != "win32":
            self.figure.set_facecolor("none")
        self.figure.subplots_adjust(left=0.18, bottom=0.08, right=0.98, top=0.95)
        self.canvas = FigureCanvas(self.figure)
        self.plotLayout.addWidget(self.canvas)
        self.ax = self.figure.add_subplot(111)
        self.ax.grid()
        x = np.arange(self.bins)
        (self.curve,) = self.ax.plot(x, self.buffer, drawstyle="steps-mid", color="#FFAA00")
        # create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, None, False)
        self.toolbar.layout().setSpacing(6)
        # remove subplots action
        actions = self.toolbar.actions()
        self.toolbar.removeAction(actions[6])
        self.toolbar.removeAction(actions[7])
        self.logCheck = QCheckBox("log scale")
        self.logCheck.setChecked(True)
        self.toolbar.addSeparator()
        self.toolbar.addWidget(self.logCheck)
        self.plotLayout.addWidget(self.toolbar)
        # configure controls
        actions[0].triggered.disconnect()
        actions[0].triggered.connect(self.home)
        self.logCheck.toggled.connect(self.set_scale)
        self.startButton.clicked.connect(self.start)
        self.loadButton.clicked.connect(self.load)
        self.canvas.mpl_connect("motion_notify_event", self.on_motion)
        # update controls
        self.set_scale(self.logCheck.isChecked())

    def start(self):
        if self.mcpha.idle:
            return
        self.mcpha.set_gen_fall(self.fallValue.value())
        self.mcpha.set_gen_rise(self.riseValue.value())
        self.mcpha.set_gen_rate(self.rateValue.value() * 1000)
        self.mcpha.set_gen_dist(self.poissonButton.isChecked())
        for value in np.arange(self.bins) << 32 | self.buffer:
            self.mcpha.set_gen_bin(value)
        self.mcpha.start_gen()
        self.startButton.setText("Stop")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.stop)
        self.log.print("generator started")

    def stop(self):
        self.mcpha.stop_gen()
        self.startButton.setText("Start")
        self.startButton.clicked.disconnect()
        self.startButton.clicked.connect(self.start)
        self.log.print("generator stopped")

    def home(self):
        self.set_scale(self.logCheck.isChecked())

    def set_scale(self, checked):
        self.toolbar.home()
        self.toolbar.update()
        if checked:
            self.ax.set_ylim(1, 1e10)
            self.ax.set_yscale("log")
        else:
            self.ax.set_ylim(auto=True)
            self.ax.set_yscale("linear")
        self.ax.relim()
        self.ax.autoscale_view(scalex=True, scaley=True)
        self.canvas.draw()

    def on_motion(self, event):
        if event.inaxes != self.ax:
            return
        x = int(event.xdata + 0.5)
        if x < 0 or x >= self.bins:
            return
        y = self.curve.get_ydata(True)[x]
        self.numberValue.setText("%d" % x)
        self.entriesValue.setText("%d" % y)

    def load(self):
        try:
            dialog = QFileDialog(self, "Load gen file", ".", "*.gen")
            dialog.setDefaultSuffix("gen")
            dialog.setAcceptMode(QFileDialog.AcceptOpen)
            if dialog.exec() == QDialog.Accepted:
                name = dialog.selectedFiles()
                self.buffer[:] = np.loadtxt(name[0], np.uint32)
                self.curve.set_ydata(self.buffer)
                self.ax.relim()
                self.ax.autoscale_view(scalex=False, scaley=True)
                self.canvas.draw()
        except:
            self.log.print("error: %s" % sys.exc_info()[1])


app = QApplication(sys.argv)
window = MCPHA()
window.show()
sys.exit(app.exec_())
