#!/usr/bin/env python

import sys
import struct

import numpy as np

import matplotlib
import matplotlib.cm as cm

from matplotlib.figure import Figure

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

if "PyQt5" in sys.modules:
    from PyQt5.uic import loadUiType
    from PyQt5.QtCore import QRegExp, QTimer, Qt
    from PyQt5.QtGui import QRegExpValidator
    from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
    from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket
else:
    from PySide2.QtUiTools import loadUiType
    from PySide2.QtCore import QRegExp, QTimer, Qt
    from PySide2.QtGui import QRegExpValidator
    from PySide2.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
    from PySide2.QtNetwork import QAbstractSocket, QTcpSocket

Ui_Scanner, QMainWindow = loadUiType("scanner.ui")


class Scanner(QMainWindow, Ui_Scanner):
    def __init__(self):
        super(Scanner, self).__init__()
        self.setupUi(self)
        # IP address validator
        rx = QRegExp("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])|rp-[0-9A-Fa-f]{6}\.local$")
        self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
        # state variable
        self.idle = True
        # number of samples to show on the plot
        self.size = 512 * 512
        self.freq = 125.0
        # buffer and offset for the incoming samples
        self.buffer = bytearray(8 * self.size)
        self.offset = 0
        self.data = np.frombuffer(self.buffer, np.int32)
        # create figure
        figure = Figure()
        figure.set_facecolor("none")
        self.axes = figure.add_subplot(111)
        self.canvas = FigureCanvas(figure)
        self.plotLayout.addWidget(self.canvas)
        self.axes.axis((0.0, 512.0, 0.0, 512.0))
        x, y = np.meshgrid(np.linspace(0.0, 512.0, 513), np.linspace(0.0, 512.0, 513))
        z = x / 512.0 + y * 0.0
        self.mesh = self.axes.pcolormesh(x, y, z, cmap=cm.plasma)
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
        self.connectButton.clicked.connect(self.start)
        self.scanButton.clicked.connect(self.scan)
        self.periodValue.valueChanged.connect(self.set_period)
        self.trgtimeValue.valueChanged.connect(self.set_trgtime)
        self.trginvCheck.stateChanged.connect(self.set_trginv)
        self.shdelayValue.valueChanged.connect(self.set_shdelay)
        self.shtimeValue.valueChanged.connect(self.set_shtime)
        self.shinvCheck.stateChanged.connect(self.set_shinv)
        self.acqdelayValue.valueChanged.connect(self.set_acqdelay)
        self.samplesValue.valueChanged.connect(self.set_samples)
        self.pulsesValue.valueChanged.connect(self.set_pulses)
        # create timers
        self.startTimer = QTimer(self)
        self.startTimer.timeout.connect(self.timeout)
        self.meshTimer = QTimer(self)
        self.meshTimer.timeout.connect(self.update_mesh)
        # set default values
        self.periodValue.setValue(200.0)

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
        self.connectButton.setText("Connect")
        self.connectButton.setEnabled(True)
        self.scanButton.setEnabled(True)

    def timeout(self):
        self.display_error("timeout")

    def connected(self):
        self.startTimer.stop()
        self.idle = False
        self.set_period(self.periodValue.value())
        self.set_trgtime(self.trgtimeValue.value())
        self.set_trginv(self.trginvCheck.checkState())
        self.set_shdelay(self.shdelayValue.value())
        self.set_shtime(self.shtimeValue.value())
        self.set_shinv(self.shinvCheck.checkState())
        self.set_acqdelay(self.acqdelayValue.value())
        self.set_samples(self.samplesValue.value())
        self.set_pulses(self.pulsesValue.value())
        # start pulse generators
        self.socket.write(struct.pack("<I", 11 << 28))
        self.connectButton.setText("Disconnect")
        self.connectButton.setEnabled(True)
        self.scanButton.setEnabled(True)

    def read_data(self):
        size = self.socket.bytesAvailable()
        if self.offset + size < 8 * self.size:
            self.buffer[self.offset : self.offset + size] = self.socket.read(size)
            self.offset += size
        else:
            self.meshTimer.stop()
            self.buffer[self.offset : 8 * self.size] = self.socket.read(8 * self.size - self.offset)
            self.offset = 0
            self.update_mesh()
            self.scanButton.setEnabled(True)

    def display_error(self, socketError):
        self.startTimer.stop()
        if socketError == "timeout":
            QMessageBox.information(self, "Scanner", "Error: connection timeout.")
        else:
            QMessageBox.information(self, "Scanner", "Error: %s." % self.socket.errorString())
        self.stop()

    def set_period(self, value):
        # set maximum delays and times to half period
        maximum = int(value * 5.0 + 0.5) / 10.0
        self.trgtimeValue.setMaximum(maximum)
        self.shdelayValue.setMaximum(maximum)
        self.shtimeValue.setMaximum(maximum)
        self.acqdelayValue.setMaximum(maximum)
        # set maximum number of samples per pulse
        maximum = int(value * 500.0 + 0.5) / 10.0
        if maximum > 256.0:
            maximum = 256.0
        self.samplesValue.setMaximum(maximum)
        shdelay = value * 0.25
        samples = value * 0.5
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 0 << 28 | int(value * self.freq)))

    def set_trgtime(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 1 << 28 | int(value * self.freq)))

    def set_trginv(self, checked):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 2 << 28 | int(checked == Qt.Checked)))

    def set_shdelay(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 3 << 28 | int(value * self.freq)))

    def set_shtime(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 4 << 28 | int(value * self.freq)))

    def set_shinv(self, checked):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 5 << 28 | int(checked == Qt.Checked)))

    def set_acqdelay(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 6 << 28 | int(value * self.freq)))

    def set_samples(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 7 << 28 | int(value)))

    def set_pulses(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 8 << 28 | int(value)))

    def set_coordinates(self):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 9 << 28))
        for i in range(256):
            for j in range(512):
                value = (i * 2 + 0 << 18) | (j << 4)
                self.socket.write(struct.pack("<I", 10 << 28 | int(value)))
            for j in range(512):
                value = (i * 2 + 1 << 18) | (511 - j << 4)
                self.socket.write(struct.pack("<I", 10 << 28 | int(value)))

    def scan(self):
        if self.idle:
            return
        self.scanButton.setEnabled(False)
        self.data[:] = np.zeros(2 * 512 * 512, np.int32)
        self.update_mesh()
        self.set_coordinates()
        self.socket.write(struct.pack("<I", 12 << 28))
        self.meshTimer.start(1000)

    def update_mesh(self):
        result = self.data[0::2] / (self.samplesValue.value() * self.pulsesValue.value() * 8192.0)
        result = result.reshape(512, 512)
        result[1::2, :] = result[1::2, ::-1]
        self.mesh.set_array(result.reshape(512 * 512))
        self.canvas.draw()


app = QApplication(sys.argv)
dpi = app.primaryScreen().logicalDotsPerInch()
matplotlib.rcParams["figure.dpi"] = dpi
window = Scanner()
window.show()
sys.exit(app.exec_())
