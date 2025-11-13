#!/usr/bin/env python

import sys
import struct

import numpy as np

import matplotlib

from matplotlib.figure import Figure

from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qtagg import NavigationToolbar2QT as NavigationToolbar

if "PyQt6" in sys.modules:
    from PyQt6.uic import loadUiType
    from PyQt6.QtCore import QRegularExpression, QTimer
    from PyQt6.QtGui import QRegularExpressionValidator
    from PyQt6.QtWidgets import QApplication, QMainWindow, QMessageBox
    from PyQt6.QtNetwork import QAbstractSocket, QTcpSocket
else:
    from PySide6.QtUiTools import loadUiType
    from PySide6.QtCore import QRegularExpression, QTimer
    from PySide6.QtGui import QRegularExpressionValidator
    from PySide6.QtWidgets import QApplication, QMainWindow, QMessageBox
    from PySide6.QtNetwork import QAbstractSocket, QTcpSocket

Ui_PulsedNMR, QMainWindow = loadUiType("pulsed_nmr.ui")


class PulsedNMR(QMainWindow, Ui_PulsedNMR):
    rates = {0: 25.0e3, 1: 50.0e3, 2: 125.0e3, 3: 250.0e3, 4: 500.0e3, 5: 1250.0e3}

    def __init__(self):
        super(PulsedNMR, self).__init__()
        self.setupUi(self)
        self.rateValue.addItems(["25", "50", "125", "250", "500", "1250"])
        # IP address validator
        rx = QRegularExpression(r"^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])|rp-[0-9A-Fa-f]{6}\.local$")
        self.addrValue.setValidator(QRegularExpressionValidator(rx, self.addrValue))
        # state variable
        self.idle = True
        # number of samples to show on the plot
        self.size = 50000
        # buffer and offset for the incoming samples
        self.buffer = bytearray(16 * self.size)
        self.offset = 0
        self.data = np.frombuffer(self.buffer, np.complex64)
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
        self.socket.errorOccurred.connect(self.display_error)
        # connect signals from buttons and boxes
        self.startButton.clicked.connect(self.start)
        self.freqValue.valueChanged.connect(self.set_freq)
        self.deltaValue.valueChanged.connect(self.set_delta)
        self.rateValue.currentIndexChanged.connect(self.set_rate)
        # set rate
        self.rateValue.setCurrentIndex(5)
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
            self.curve.set_ydata(np.abs(self.data[0::2]))
            self.canvas.draw()

    def display_error(self, socketError):
        self.startTimer.stop()
        if socketError == "timeout":
            QMessageBox.information(self, "PulsedNMR", "Error: connection timeout.")
        else:
            QMessageBox.information(self, "PulsedNMR", "Error: %s." % self.socket.errorString())
        self.stop()

    def send_command(self, code, data):
        self.socket.write(struct.pack("<Q", int(code) << 60 | int(data)))

    def set_freq(self, value):
        if self.idle:
            return
        freq = int(1.0e6 * value + 0.5)
        self.send_command(0, freq << 30 | freq)

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
        self.send_command(1, 125.0e6 / rate / 2)

    def set_delta(self, value):
        if self.idle:
            return
        self.timer.stop()
        self.timer.start(value)

    def clear_events(self):
        if self.idle:
            return
        self.send_command(6, 0)

    def add_event(self, delay, sync=0, gate=0, level=0, tx_phase=0, rx_phase=0):
        lvl = int(level / 100.0 * 32766 + 0.5)
        txp = int(tx_phase / 360.0 * 0x3FFFFFFF + 0.5)
        rxp = int(rx_phase / 360.0 * 0x3FFFFFFF + 0.5)
        self.send_command(7, lvl << 44 | gate << 41 | sync << 40 | int(delay - 1))
        self.send_command(8, rxp << 30 | txp)

    def start_sequence(self):
        if self.idle:
            return
        awidth = np.floor(125.0 * self.awidthValue.value() + 0.5)
        bwidth = np.floor(125.0 * self.bwidthValue.value() + 0.5)
        delay = np.floor(125.0 * self.delayValue.value() + 0.5)
        size = self.size
        self.clear_events()
        self.add_event(delay=1, sync=1)
        self.add_event(delay=awidth, gate=1, level=100)
        self.add_event(delay=delay)
        self.add_event(delay=bwidth, gate=1, level=100)
        self.send_command(9, 1 << 40 | int(size - 1))
        self.send_command(10, size)


app = QApplication(sys.argv)
dpi = app.primaryScreen().logicalDotsPerInch()
matplotlib.rcParams["figure.dpi"] = dpi
window = PulsedNMR()
window.show()
sys.exit(app.exec())
