#!/usr/bin/env python

import sys
import struct

import numpy as np

import matplotlib

from matplotlib.figure import Figure

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

if "PyQt5" in sys.modules:
    from PyQt5.uic import loadUiType
    from PyQt5.QtCore import QRegExp, QTimer
    from PyQt5.QtGui import QRegExpValidator
    from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
    from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket
else:
    from PySide2.QtUiTools import loadUiType
    from PySide2.QtCore import QRegExp, QTimer
    from PySide2.QtGui import QRegExpValidator
    from PySide2.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
    from PySide2.QtNetwork import QAbstractSocket, QTcpSocket

Ui_PulsedNMR, QMainWindow = loadUiType("pulsed_nmr.ui")


class PulsedNMR(QMainWindow, Ui_PulsedNMR):
    rates = {0: 25.0e3, 1: 50.0e3, 2: 125.0e3, 3: 250.0e3, 4: 500.0e3, 5: 1250.0e3}

    def __init__(self):
        super(PulsedNMR, self).__init__()
        self.setupUi(self)
        self.rateValue.addItems(["25", "50", "125", "250", "500", "1250"])
        # IP address validator
        rx = QRegExp("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])|rp-[0-9A-Fa-f]{6}\.local$")
        self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
        # state variable
        self.idle = True
        # number of samples to show on the plot
        self.size = 50000
        # buffer and offset for the incoming samples
        self.buffer = bytearray(16 * self.size)
        self.offset = 0
        self.data = np.frombuffer(self.buffer, np.int32)
        # create figure
        figure = Figure()
        figure.set_facecolor("none")
        self.axes = figure.add_subplot(111)
        self.canvas = FigureCanvas(figure)
        self.plotLayout.addWidget(self.canvas)
        # create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, self.plotWidget, False)
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
        self.startButton.clicked.connect(self.start)
        self.freqValue.valueChanged.connect(self.set_freq)
        self.deltaValue.valueChanged.connect(self.set_delta)
        self.rateValue.currentIndexChanged.connect(self.set_rate)
        # set rate
        self.rateValue.setCurrentIndex(3)
        # create timer for the repetitions
        self.startTimer = QTimer(self)
        self.startTimer.timeout.connect(self.timeout)
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.start_sequence)

    def start(self):
        if self.idle:
            self.startButton.setEnabled(False)
            self.socket.connectToHost(self.addrValue.text(), 1001)
            self.startTimer.start(5000)
        else:
            self.stop()

    def stop(self):
        self.idle = True
        self.timer.stop()
        self.socket.abort()
        self.offset = 0
        self.startButton.setText("Start")
        self.startButton.setEnabled(True)

    def timeout(self):
        self.display_error("timeout")

    def connected(self):
        self.startTimer.stop()
        self.idle = False
        self.set_freq(self.freqValue.value())
        self.set_rate(self.rateValue.currentIndex())
        self.start_sequence()
        self.timer.start(self.deltaValue.value())
        self.startButton.setText("Stop")
        self.startButton.setEnabled(True)

    def read_data(self):
        size = self.socket.bytesAvailable()
        if self.offset + size < 16 * self.size:
            self.buffer[self.offset : self.offset + size] = self.socket.read(size)
            self.offset += size
        else:
            self.buffer[self.offset : 16 * self.size] = self.socket.read(16 * self.size - self.offset)
            self.offset = 0
            # plot the signal envelope
            self.curve.set_ydata(np.abs(self.data.astype(np.float32).view(np.complex64)[0::2] / (1 << 30)))
            self.canvas.draw()

    def display_error(self, socketError):
        self.startTimer.stop()
        if socketError == "timeout":
            QMessageBox.information(self, "PulsedNMR", "Error: connection timeout.")
        else:
            QMessageBox.information(self, "PulsedNMR", "Error: %s." % self.socket.errorString())
        self.stop()

    def set_freq(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<Q", 0 << 60 | int(1.0e6 * value)))
        self.socket.write(struct.pack("<Q", 1 << 60 | int(1.0e6 * value)))

    def set_rate(self, index):
        # time axis
        rate = float(PulsedNMR.rates[index])
        time = np.linspace(0.0, (self.size - 1) * 1000.0 / rate, self.size)
        # reset toolbar
        self.toolbar.home()
        self.toolbar.update()
        # reset plot
        self.axes.clear()
        self.axes.grid()
        # plot zeros and get store the returned Line2D object
        (self.curve,) = self.axes.plot(time, np.zeros(self.size))
        x1, x2, y1, y2 = self.axes.axis()
        # set y axis limits
        self.axes.axis((x1, x2, -0.1, 1.1))
        self.axes.set_xlabel("time, ms")
        self.canvas.draw()
        if self.idle:
            return
        self.socket.write(struct.pack("<Q", 2 << 60 | int(125.0e6 / rate / 2)))

    def set_delta(self, value):
        if self.idle:
            return
        self.timer.stop()
        self.timer.start(value)

    def clear_pulses(self):
        if self.idle:
            return
        self.socket.write(struct.pack("<Q", 7 << 60))

    def add_delay(self, gate, width):
        if self.idle:
            return
        self.socket.write(struct.pack("<Q", 8 << 60 | int(width - 1)))
        self.socket.write(struct.pack("<Q", 9 << 60 | int(gate << 48)))

    def add_pulse(self, level, phase, width):
        if self.idle:
            return
        phase = int(np.floor(phase / 360.0 * (1 << 30) + 0.5))
        self.socket.write(struct.pack("<Q", 8 << 60 | int(width - 1)))
        self.socket.write(struct.pack("<Q", 9 << 60 | int(1 << 48 | level << 32 | phase)))

    def start_sequence(self):
        if self.idle:
            return
        awidth = np.floor(125 * self.awidthValue.value() + 0.5)
        bwidth = np.floor(125 * self.bwidthValue.value() + 0.5)
        delay = np.floor(125 * self.delayValue.value() + 0.5)
        size = self.size
        self.clear_pulses()
        self.add_pulse(32766, 0, awidth)
        self.add_delay(0, delay)
        self.add_pulse(32766, 0, bwidth)
        self.socket.write(struct.pack("<Q", 10 << 60 | int(size)))


app = QApplication(sys.argv)
dpi = app.primaryScreen().logicalDotsPerInch()
matplotlib.rcParams["figure.dpi"] = dpi
window = PulsedNMR()
window.show()
sys.exit(app.exec_())
