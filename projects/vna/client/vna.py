#!/usr/bin/env python3

import sys
import struct
import warnings

from functools import partial

import numpy as np

import matplotlib

from matplotlib.figure import Figure
from matplotlib.ticker import Formatter, FuncFormatter

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.backends.backend_qt5agg import NavigationToolbar2QT as NavigationToolbar

if "PyQt5" in sys.modules:
    from PyQt5.uic import loadUiType
    from PyQt5.QtCore import QRegExp, QTimer, QSettings, QDir, Qt
    from PyQt5.QtGui import QRegExpValidator
    from PyQt5.QtWidgets import QApplication, QMainWindow, QMessageBox, QDialog, QFileDialog, QPushButton, QLabel, QSpinBox
    from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket
else:
    from PySide2.QtUiTools import loadUiType
    from PySide2.QtCore import QRegExp, QTimer, QSettings, QDir, Qt
    from PySide2.QtGui import QRegExpValidator
    from PySide2.QtWidgets import QApplication, QMainWindow, QMessageBox, QDialog, QFileDialog, QPushButton, QLabel, QSpinBox
    from PySide2.QtNetwork import QAbstractSocket, QTcpSocket

Ui_VNA, QMainWindow = loadUiType("vna.ui")


def unicode_minus(s):
    return s.replace("-", "\u2212")


def metric_prefix(x, pos=None):
    if x == 0.0:
        s = "0"
    elif abs(x) >= 1.0e5:
        if x > 0.0:
            s = "99.9k"
        else:
            s = "-99.9k"
    elif abs(x) < 1.0e-2:
        if x > 0.0:
            s = "9.99m"
        else:
            s = "-9.99m"
    elif abs(x) >= 1.0e3:
        s = "%.3gk" % (x * 1.0e-3)
    elif abs(x) >= 1.0e0:
        s = "%.3g" % x
    elif abs(x) >= 1.0e-3:
        s = "%.3gm" % (x * 1e3)
    else:
        s = "%.3g" % x
    return unicode_minus(s)


class Measurement:
    def __init__(self, start, stop, size):
        self.freq = np.linspace(start, stop, size)
        self.data = np.zeros(size, np.complex64)
        self.period = 62500


class FigureTab:
    cursors = [15000, 35000]
    colors = ["orange", "violet"]

    def __init__(self, layout, vna):
        # create figure
        self.figure = Figure()
        if sys.platform != "win32":
            self.figure.set_facecolor("none")
        self.canvas = FigureCanvas(self.figure)
        layout.addWidget(self.canvas)
        # create navigation toolbar
        self.toolbar = NavigationToolbar(self.canvas, None, False)
        self.toolbar.layout().setSpacing(6)
        # remove subplots action
        actions = self.toolbar.actions()
        if int(matplotlib.__version__[0]) < 2:
            self.toolbar.removeAction(actions[7])
        else:
            self.toolbar.removeAction(actions[6])
        self.toolbar.addSeparator()
        self.cursorLabels = {}
        self.cursorValues = {}
        self.cursorMarkers = {}
        self.cursorPressed = {}
        for i in range(len(self.cursors)):
            self.cursorMarkers[i] = None
            self.cursorPressed[i] = False
            self.cursorLabels[i] = QLabel("Cursor %d, kHz" % (i + 1))
            self.cursorLabels[i].setStyleSheet("color: %s" % self.colors[i])
            self.cursorValues[i] = QSpinBox()
            self.cursorValues[i].setMinimumSize(90, 0)
            self.cursorValues[i].setSingleStep(10)
            self.cursorValues[i].setAlignment(Qt.AlignRight | Qt.AlignTrailing | Qt.AlignVCenter)
            self.toolbar.addWidget(self.cursorLabels[i])
            self.toolbar.addWidget(self.cursorValues[i])
            self.cursorValues[i].valueChanged.connect(partial(self.set_cursor, i))
            self.canvas.mpl_connect("button_press_event", partial(self.press_marker, i))
            self.canvas.mpl_connect("motion_notify_event", partial(self.move_marker, i))
            self.canvas.mpl_connect("button_release_event", partial(self.release_marker, i))
        self.toolbar.addSeparator()
        self.plotButton = QPushButton("Rescale")
        self.toolbar.addWidget(self.plotButton)
        layout.addWidget(self.toolbar)
        self.plotButton.clicked.connect(self.plot)
        self.mode = None
        self.vna = vna

    def add_cursors(self, axes):
        if self.mode == "gain_short" or self.mode == "gain_open":
            columns = ["Freq., kHz", "G, dB", r"$\angle$ G, deg"]
        else:
            columns = ["Freq., kHz", "Re(Z), \u03A9", "Im(Z), \u03A9", "|Z|, \u03A9", r"$\angle$ Z, deg", "SWR", r"|$\Gamma$|", r"$\angle$ $\Gamma$, deg", "RL, dB"]
        y = len(self.cursors) * 0.04 + 0.01
        for i in range(len(columns)):
            self.figure.text(0.19 + 0.1 * i, y, columns[i], horizontalalignment="right")
        self.cursorRows = {}
        for i in range(len(self.cursors)):
            y = len(self.cursors) * 0.04 - 0.03 - 0.04 * i
            self.figure.text(0.01, y, "Cursor %d" % (i + 1), color=self.colors[i])
            self.cursorRows[i] = {}
            for j in range(len(columns)):
                self.cursorRows[i][j] = self.figure.text(0.19 + 0.1 * j, y, "", horizontalalignment="right")
            if self.mode == "smith":
                (self.cursorMarkers[i],) = axes.plot(0.0, 0.0, marker="o", color=self.colors[i])
            else:
                self.cursorMarkers[i] = axes.axvline(0.0, color=self.colors[i], linewidth=2)
            self.set_cursor(i, self.cursorValues[i].value())

    def set_cursor(self, index, value):
        FigureTab.cursors[index] = value
        marker = self.cursorMarkers[index]
        if marker is None:
            return
        row = self.cursorRows[index]
        freq = value
        gamma = self.vna.gamma(freq)
        if self.mode == "smith":
            marker.set_xdata(gamma.real)
            marker.set_ydata(gamma.imag)
        else:
            marker.set_xdata(freq)
        row[0].set_text("%d" % freq)
        if self.mode == "gain_short":
            gain = self.vna.gain_short(freq)
            magnitude = 20.0 * np.log10(np.absolute(gain))
            angle = np.angle(gain, deg=True)
            row[1].set_text(unicode_minus("%.2f" % magnitude))
            row[2].set_text(unicode_minus("%.1f" % angle))
        elif self.mode == "gain_open":
            gain = self.vna.gain_open(freq)
            magnitude = 20.0 * np.log10(np.absolute(gain))
            angle = np.angle(gain, deg=True)
            row[1].set_text(unicode_minus("%.2f" % magnitude))
            row[2].set_text(unicode_minus("%.1f" % angle))
        else:
            swr = self.vna.swr(freq)
            z = self.vna.impedance(freq)
            rl = 20.0 * np.log10(np.absolute(gamma))
            if rl > -0.01:
                rl = 0.0
            row[1].set_text(metric_prefix(z.real))
            row[2].set_text(metric_prefix(z.imag))
            row[3].set_text(metric_prefix(np.absolute(z)))
            angle = np.angle(z, deg=True)
            if np.abs(angle) < 0.1:
                angle = 0.0
            row[4].set_text(unicode_minus("%.1f" % angle))
            row[5].set_text(unicode_minus("%.2f" % swr))
            row[6].set_text(unicode_minus("%.2f" % np.absolute(gamma)))
            angle = np.angle(gamma, deg=True)
            if np.abs(angle) < 0.1:
                angle = 0.0
            row[7].set_text(unicode_minus("%.1f" % angle))
            row[8].set_text(unicode_minus("%.2f" % rl))
        self.canvas.draw()

    def press_marker(self, index, event):
        if not event.inaxes:
            return
        if self.mode == "smith":
            return
        marker = self.cursorMarkers[index]
        if marker is None:
            return
        contains, misc = marker.contains(event)
        if not contains:
            return
        self.cursorPressed[index] = True

    def move_marker(self, index, event):
        if not event.inaxes:
            return
        if self.mode == "smith":
            return
        if not self.cursorPressed[index]:
            return
        self.cursorValues[index].setValue(event.xdata)

    def release_marker(self, index, event):
        self.cursorPressed[index] = False

    def xlim(self, freq):
        start = freq[0]
        stop = freq[-1]
        min = np.minimum(start, stop)
        max = np.maximum(start, stop)
        margin = (max - min) / 50
        return (min - margin, max + margin)

    def plot(self):
        getattr(self, "plot_%s" % self.mode)()

    def update(self, mode):
        start = self.vna.dut.freq[0]
        stop = self.vna.dut.freq[-1]
        min = int(np.minimum(start, stop))
        max = int(np.maximum(start, stop))
        for i in range(len(self.cursors)):
            value = self.cursors[i]
            self.cursorValues[i].setRange(min, max)
            self.cursorValues[i].setValue(value)
            value = self.cursorValues[i].value()
            self.set_cursor(i, value)
        getattr(self, "update_%s" % mode)()

    def plot_curves(self, freq, data1, label1, limit1, data2, label2, limit2):
        matplotlib.rcdefaults()
        matplotlib.rcParams["axes.formatter.use_mathtext"] = True
        self.figure.clf()
        bottom = len(self.cursors) * 0.04 + 0.13
        self.figure.subplots_adjust(left=0.16, bottom=bottom, right=0.84, top=0.96)
        axes1 = self.figure.add_subplot(111)
        axes1.cla()
        axes1.xaxis.grid()
        axes1.set_xlabel("kHz")
        axes1.set_ylabel(label1)
        xlim = self.xlim(freq)
        axes1.set_xlim(xlim)
        if limit1 is not None:
            axes1.set_ylim(limit1)
        (self.curve1,) = axes1.plot(freq, data1, color="blue", label=label1)
        self.add_cursors(axes1)
        if data2 is None:
            self.canvas.draw()
            return
        axes1.tick_params("y", color="blue", labelcolor="blue")
        axes1.yaxis.label.set_color("blue")
        axes2 = axes1.twinx()
        axes2.spines["left"].set_color("blue")
        axes2.spines["right"].set_color("red")
        axes2.set_ylabel(label2)
        axes2.set_xlim(xlim)
        if limit2 is not None:
            axes2.set_ylim(limit2)
        axes2.tick_params("y", color="red", labelcolor="red")
        axes2.yaxis.label.set_color("red")
        (self.curve2,) = axes2.plot(freq, data2, color="red", label=label2)
        self.canvas.draw()

    def plot_gain(self, gain):
        freq = self.vna.dut.freq
        data1 = 20.0 * np.log10(np.absolute(gain))
        data2 = np.angle(gain, deg=True)
        self.plot_curves(freq, data1, "G, dB", (-110, 110.0), data2, r"$\angle$ G, deg", (-198, 198))

    def plot_gain_short(self):
        self.mode = "gain_short"
        self.plot_gain(self.vna.gain_short(self.vna.dut.freq))

    def plot_gain_open(self):
        self.mode = "gain_open"
        self.plot_gain(self.vna.gain_open(self.vna.dut.freq))

    def update_gain(self, gain, mode):
        if self.mode == mode:
            self.curve1.set_xdata(self.vna.dut.freq)
            self.curve1.set_ydata(20.0 * np.log10(np.absolute(gain)))
            self.curve2.set_xdata(self.vna.dut.freq)
            self.curve2.set_ydata(np.angle(gain, deg=True))
            self.canvas.draw()
        else:
            self.mode = mode
            self.plot_gain(gain)

    def update_gain_short(self):
        self.update_gain(self.vna.gain_short(self.vna.dut.freq), "gain_short")

    def update_gain_open(self):
        self.update_gain(self.vna.gain_open(self.vna.dut.freq), "gain_open")

    def plot_magphase(self, freq, data, label, mode):
        self.mode = mode
        data1 = np.absolute(data)
        data2 = np.angle(data, deg=True)
        max = np.fmax(0.01, data1.max())
        label1 = r"|%s|" % label
        label2 = r"$\angle$ %s, deg" % label
        self.plot_curves(freq, data1, label1, (-0.05 * max, 1.05 * max), data2, label2, (-198, 198))

    def update_magphase(self, freq, data, label, mode):
        if self.mode == mode:
            self.curve1.set_xdata(freq)
            self.curve1.set_ydata(np.absolute(data))
            self.curve2.set_xdata(freq)
            self.curve2.set_ydata(np.angle(data, deg=True))
            self.canvas.draw()
        else:
            self.plot_magphase(freq, data, label, mode)

    def plot_open(self):
        self.plot_magphase(self.vna.open.freq, self.vna.open.data, "open", "open")

    def update_open(self):
        self.update_magphase(self.vna.open.freq, self.vna.open.data, "open", "open")

    def plot_short(self):
        self.plot_magphase(self.vna.short.freq, self.vna.short.data, "short", "short")

    def update_short(self):
        self.update_magphase(self.vna.short.freq, self.vna.short.data, "short", "short")

    def plot_load(self):
        self.plot_magphase(self.vna.load.freq, self.vna.load.data, "load", "load")

    def update_load(self):
        self.update_magphase(self.vna.load.freq, self.vna.load.data, "load", "load")

    def plot_dut(self):
        self.plot_magphase(self.vna.dut.freq, self.vna.dut.data, "dut", "dut")

    def update_dut(self):
        self.update_magphase(self.vna.dut.freq, self.vna.dut.data, "dut", "dut")

    def plot_smith_grid(self, axes, color):
        load = 50.0
        ticks = np.array([0.0, 0.2, 0.5, 1.0, 2.0, 5.0])
        for tick in ticks * load:
            axis = np.logspace(-4, np.log10(1.0e3), 200) * load
            z = tick + 1.0j * axis
            gamma = (z - load) / (z + load)
            axes.plot(gamma.real, gamma.imag, color=color, linewidth=0.4, alpha=0.3)
            axes.plot(gamma.real, -gamma.imag, color=color, linewidth=0.4, alpha=0.3)
            z = axis + 1.0j * tick
            gamma = (z - load) / (z + load)
            axes.plot(gamma.real, gamma.imag, color=color, linewidth=0.4, alpha=0.3)
            axes.plot(gamma.real, -gamma.imag, color=color, linewidth=0.4, alpha=0.3)
            if tick == 0.0:
                axes.text(1.0, 0.0, "\u221E", color=color, ha="left", va="center", clip_on=True, fontsize="x-large")
                axes.text(-1.0, 0.0, "0\u03A9", color=color, ha="left", va="bottom", clip_on=True)
                continue
            lab = "%d\u03A9" % tick
            x = (tick - load) / (tick + load)
            axes.text(x, 0.0, lab, color=color, ha="left", va="bottom", clip_on=True)
            lab = "j%d\u03A9" % tick
            z = 1.0j * tick
            gamma = (z - load) / (z + load) * 1.05
            x = gamma.real
            y = gamma.imag
            angle = np.angle(gamma) * 180.0 / np.pi - 90.0
            axes.text(x, y, lab, color=color, ha="center", va="center", clip_on=True, rotation=angle)
            lab = "\u2212j%d\u03A9" % tick
            axes.text(x, -y, lab, color=color, ha="center", va="center", clip_on=True, rotation=-angle)

    def plot_smith(self):
        self.mode = "smith"
        matplotlib.rcdefaults()
        self.figure.clf()
        bottom = len(self.cursors) * 0.04 + 0.05
        self.figure.subplots_adjust(left=0.0, bottom=bottom, right=1.0, top=1.0)
        axes1 = self.figure.add_subplot(111)
        self.plot_smith_grid(axes1, "blue")
        gamma = self.vna.gamma(self.vna.dut.freq)
        (self.curve1,) = axes1.plot(gamma.real, gamma.imag, color="red")
        axes1.axis("equal")
        axes1.set_xlim(-1.12, 1.12)
        axes1.set_ylim(-1.12, 1.12)
        axes1.xaxis.set_visible(False)
        axes1.yaxis.set_visible(False)
        for loc, spine in axes1.spines.items():
            spine.set_visible(False)
        self.add_cursors(axes1)
        self.canvas.draw()

    def update_smith(self):
        if self.mode == "smith":
            gamma = self.vna.gamma(self.vna.dut.freq)
            self.curve1.set_xdata(gamma.real)
            self.curve1.set_ydata(gamma.imag)
            self.canvas.draw()
        else:
            self.plot_smith()

    def plot_imp(self):
        self.mode = "imp"
        freq = self.vna.dut.freq
        z = self.vna.impedance(freq)
        data1 = np.fmin(9.99e4, np.absolute(z))
        data2 = np.angle(z, deg=True)
        max = np.fmax(0.01, data1.max())
        self.plot_curves(freq, data1, "|Z|, \u03A9", (-0.05 * max, 1.05 * max), data2, r"$\angle$ Z, deg", (-198, 198))

    def update_imp(self):
        if self.mode == "imp":
            freq = self.vna.dut.freq
            z = self.vna.impedance(freq)
            data1 = np.fmin(9.99e4, np.absolute(z))
            data2 = np.angle(z, deg=True)
            self.curve1.set_xdata(freq)
            self.curve1.set_ydata(data1)
            self.curve2.set_xdata(freq)
            self.curve2.set_ydata(data2)
            self.canvas.draw()
        else:
            self.plot_imp()

    def plot_swr(self):
        self.mode = "swr"
        freq = self.vna.dut.freq
        data1 = self.vna.swr(freq)
        self.plot_curves(freq, data1, "SWR", (0.9, 3.1), None, None, None)

    def update_swr(self):
        if self.mode == "swr":
            self.curve1.set_xdata(self.vna.dut.freq)
            self.curve1.set_ydata(self.vna.swr(self.vna.dut.freq))
            self.canvas.draw()
        else:
            self.plot_swr()

    def plot_gamma(self):
        self.plot_magphase(self.vna.dut.freq, self.vna.gamma(self.vna.dut.freq), r"$\Gamma$", "gamma")

    def update_gamma(self):
        self.update_magphase(self.vna.dut.freq, self.vna.gamma(self.vna.dut.freq), r"$\Gamma$", "gamma")

    def plot_rl(self):
        self.mode = "rl"
        freq = self.vna.dut.freq
        gamma = self.vna.gamma(freq)
        data1 = 20.0 * np.log10(np.absolute(gamma))
        self.plot_curves(freq, data1, "RL, dB", (-105, 5.0), None, None, None)

    def update_rl(self):
        if self.mode == "rl":
            freq = self.vna.dut.freq
            gamma = self.vna.gamma(freq)
            data1 = 20.0 * np.log10(np.absolute(gamma))
            self.curve1.set_xdata(freq)
            self.curve1.set_ydata(data1)
            self.canvas.draw()
        else:
            self.plot_rl()


class VNA(QMainWindow, Ui_VNA):
    graphs = ["open", "short", "load", "dut", "smith", "imp", "swr", "gamma", "rl", "gain_short", "gain_open"]

    def __init__(self):
        super(VNA, self).__init__()
        self.setupUi(self)
        # address validator
        rx = QRegExp("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])|rp-[0-9A-Fa-f]{6}\.local$")
        self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
        # state variables
        self.idle = True
        self.reading = False
        self.auto = False
        # sweep parameters
        self.sweep_start = 10
        self.sweep_stop = 50000
        self.sweep_size = 5000
        # buffer and offset for the incoming samples
        self.buffer = bytearray(16 * 32768)
        self.offset = 0
        self.data = np.frombuffer(self.buffer, np.complex64)
        # create measurements
        self.open = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
        self.short = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
        self.load = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
        self.dut = Measurement(self.sweep_start, self.sweep_stop, self.sweep_size)
        self.mode = "open"
        # create figures
        self.tabs = {}
        for i in range(len(self.graphs)):
            layout = getattr(self, "%sLayout" % self.graphs[i])
            self.tabs[i] = FigureTab(layout, self)
        # configure widgets
        self.rateValue.addItems(["5000", "1000", "500", "100", "50", "10", "5", "1"])
        self.rateValue.lineEdit().setReadOnly(True)
        self.rateValue.lineEdit().setAlignment(Qt.AlignRight)
        for i in range(self.rateValue.count()):
            self.rateValue.setItemData(i, Qt.AlignRight, Qt.TextAlignmentRole)
        self.set_enabled(False)
        self.stopSweep.setEnabled(False)
        # read settings
        settings = QSettings("vna.ini", QSettings.IniFormat)
        self.read_cfg_settings(settings)
        # create TCP socket
        self.socket = QTcpSocket(self)
        self.socket.connected.connect(self.connected)
        self.socket.readyRead.connect(self.read_data)
        self.socket.error.connect(self.display_error)
        # connect signals from widgets
        self.connectButton.clicked.connect(self.start)
        self.writeButton.clicked.connect(self.write_cfg)
        self.readButton.clicked.connect(self.read_cfg)
        self.openSweep.clicked.connect(partial(self.sweep, "open"))
        self.shortSweep.clicked.connect(partial(self.sweep, "short"))
        self.loadSweep.clicked.connect(partial(self.sweep, "load"))
        self.singleSweep.clicked.connect(partial(self.sweep, "dut"))
        self.autoSweep.clicked.connect(self.sweep_auto)
        self.stopSweep.clicked.connect(self.cancel)
        self.csvButton.clicked.connect(self.write_csv)
        self.s1pButton.clicked.connect(self.write_s1p)
        self.s2pshortButton.clicked.connect(self.write_s2p_short)
        self.s2popenButton.clicked.connect(self.write_s2p_open)
        self.startValue.valueChanged.connect(self.set_start)
        self.stopValue.valueChanged.connect(self.set_stop)
        self.sizeValue.valueChanged.connect(self.set_size)
        self.rateValue.currentIndexChanged.connect(self.set_rate)
        self.corrValue.valueChanged.connect(self.set_corr)
        self.phase1Value.valueChanged.connect(self.set_phase1)
        self.phase2Value.valueChanged.connect(self.set_phase2)
        self.level1Value.valueChanged.connect(self.set_level1)
        self.level2Value.valueChanged.connect(self.set_level2)
        self.tabWidget.currentChanged.connect(self.update_tab)
        # create timers
        self.startTimer = QTimer(self)
        self.startTimer.timeout.connect(self.timeout)
        self.sweepTimer = QTimer(self)
        self.sweepTimer.timeout.connect(self.sweep_timeout)

    def set_enabled(self, enabled):
        widgets = [
            self.rateValue,
            self.level1Value,
            self.level2Value,
            self.corrValue,
            self.phase1Value,
            self.phase2Value,
            self.startValue,
            self.stopValue,
            self.sizeValue,
            self.openSweep,
            self.shortSweep,
            self.loadSweep,
            self.singleSweep,
            self.autoSweep,
        ]
        for entry in widgets:
            entry.setEnabled(enabled)

    def start(self):
        if self.idle:
            self.connectButton.setEnabled(False)
            self.socket.connectToHost(self.addrValue.text(), 1001)
            self.startTimer.start(5000)
        else:
            self.stop()

    def stop(self):
        self.idle = True
        self.cancel()
        self.socket.abort()
        self.connectButton.setText("Connect")
        self.connectButton.setEnabled(True)
        self.set_enabled(False)
        self.stopSweep.setEnabled(False)

    def timeout(self):
        self.display_error("timeout")

    def connected(self):
        self.startTimer.stop()
        self.idle = False
        self.set_rate(self.rateValue.currentIndex())
        self.set_corr(self.corrValue.value())
        self.set_phase1(self.phase1Value.value())
        self.set_phase2(self.phase2Value.value())
        self.set_level1(self.level1Value.value())
        self.set_level2(self.level2Value.value())
        self.set_gpio(1)
        self.connectButton.setText("Disconnect")
        self.connectButton.setEnabled(True)
        self.set_enabled(True)
        self.stopSweep.setEnabled(True)

    def read_data(self):
        while self.socket.bytesAvailable() > 0:
            if not self.reading:
                self.socket.readAll()
                return
            size = self.socket.bytesAvailable()
            self.progressBar.setValue((self.offset + size) // 16)
            limit = 16 * self.sweep_size
            if self.offset + size < limit:
                self.buffer[self.offset : self.offset + size] = self.socket.read(size)
                self.offset += size
            else:
                self.buffer[self.offset : limit] = self.socket.read(limit - self.offset)
                adc1 = self.data[0::2]
                adc2 = self.data[1::2]
                attr = getattr(self, self.mode)
                start = self.sweep_start
                stop = self.sweep_stop
                size = self.sweep_size
                attr.freq = np.linspace(start, stop, size)
                attr.data = adc1[0:size].copy()
                self.update_tab()
                self.reading = False
                if not self.auto:
                    self.progressBar.setValue(0)
                    self.set_enabled(True)

    def display_error(self, socketError):
        self.startTimer.stop()
        if socketError == "timeout":
            QMessageBox.information(self, "VNA", "Error: connection timeout.")
        else:
            QMessageBox.information(self, "VNA", "Error: %s." % self.socket.errorString())
        self.stop()

    def set_start(self, value):
        self.sweep_start = value

    def set_stop(self, value):
        self.sweep_stop = value

    def set_size(self, value):
        self.sweep_size = value

    def set_rate(self, value):
        if self.idle:
            return
        rate = [10, 50, 100, 500, 1000, 5000, 10000, 50000][value]
        self.socket.write(struct.pack("<I", 3 << 28 | int(rate)))

    def set_corr(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 4 << 28 | int(value & 0xFFFFFFF)))

    def set_phase1(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 5 << 28 | int(value)))

    def set_phase2(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 6 << 28 | int(value)))

    def set_level1(self, value):
        if self.idle:
            return
        data = 0 if value == -90 else int(32766 * np.power(10.0, value / 20.0))
        self.socket.write(struct.pack("<I", 7 << 28 | int(data)))

    def set_level2(self, value):
        if self.idle:
            return
        data = 0 if value == -90 else int(32766 * np.power(10.0, value / 20.0))
        self.socket.write(struct.pack("<I", 8 << 28 | int(data)))

    def set_gpio(self, value):
        if self.idle:
            return
        self.socket.write(struct.pack("<I", 9 << 28 | int(value)))

    def sweep(self, mode):
        if self.idle:
            return
        self.set_enabled(False)
        self.mode = mode
        self.offset = 0
        self.reading = True
        self.socket.write(struct.pack("<I", 0 << 28 | int(self.sweep_start * 1000)))
        self.socket.write(struct.pack("<I", 1 << 28 | int(self.sweep_stop * 1000)))
        self.socket.write(struct.pack("<I", 2 << 28 | int(self.sweep_size)))
        self.socket.write(struct.pack("<I", 10 << 28))
        self.progressBar.setMinimum(0)
        self.progressBar.setMaximum(self.sweep_size)
        self.progressBar.setValue(0)

    def cancel(self):
        self.sweepTimer.stop()
        self.auto = False
        self.reading = False
        self.socket.write(struct.pack("<I", 11 << 28))
        self.progressBar.setValue(0)
        self.set_enabled(True)

    def sweep_auto(self):
        self.auto = True
        self.sweepTimer.start(100)

    def sweep_timeout(self):
        if not self.reading:
            self.sweep("dut")

    def update_tab(self):
        index = self.tabWidget.currentIndex()
        self.tabs[index].update(self.graphs[index])

    def interp(self, freq, meas):
        real = np.interp(freq, meas.freq, meas.data.real, period=meas.period)
        imag = np.interp(freq, meas.freq, meas.data.imag, period=meas.period)
        return real + 1j * imag

    def gain_short(self, freq):
        short = self.interp(freq, self.short)
        dut = self.interp(freq, self.dut)
        return np.divide(dut, short)

    def gain_open(self, freq):
        open = self.interp(freq, self.open)
        dut = self.interp(freq, self.dut)
        return np.divide(dut, open)

    def impedance(self, freq):
        open = self.interp(freq, self.open)
        short = self.interp(freq, self.short)
        load = self.interp(freq, self.load)
        dut = self.interp(freq, self.dut)
        z = np.divide(50.0 * (open - load) * (dut - short), (load - short) * (open - dut))
        z = np.asarray(z)
        z.real[z.real < 1.0e-2] = 9.99e-3
        return z

    def gamma(self, freq):
        z = self.impedance(freq)
        return np.divide(z - 50.0, z + 50.0)

    def swr(self, freq):
        magnitude = np.absolute(self.gamma(freq))
        swr = np.divide(1.0 + magnitude, 1.0 - magnitude)
        return np.clip(swr, 1.0, 99.99)

    def write_cfg(self):
        dialog = QFileDialog(self, "Write configuration settings", ".", "*.ini")
        dialog.setDefaultSuffix("ini")
        dialog.selectFile("vna.ini")
        dialog.setAcceptMode(QFileDialog.AcceptSave)
        dialog.setOptions(QFileDialog.DontConfirmOverwrite)
        if dialog.exec() == QDialog.Accepted:
            name = dialog.selectedFiles()
            settings = QSettings(name[0], QSettings.IniFormat)
            self.write_cfg_settings(settings)

    def read_cfg(self):
        dialog = QFileDialog(self, "Read configuration settings", ".", "*.ini")
        dialog.setDefaultSuffix("ini")
        dialog.selectFile("vna.ini")
        dialog.setAcceptMode(QFileDialog.AcceptOpen)
        if dialog.exec() == QDialog.Accepted:
            name = dialog.selectedFiles()
            settings = QSettings(name[0], QSettings.IniFormat)
            self.read_cfg_settings(settings)
            window.update_tab()

    def write_cfg_settings(self, settings):
        settings.setValue("addr", self.addrValue.text())
        settings.setValue("rate", self.rateValue.currentIndex())
        settings.setValue("corr", self.corrValue.value())
        settings.setValue("phase_1", self.phase1Value.value())
        settings.setValue("phase_2", self.phase2Value.value())
        settings.setValue("level_1", self.level1Value.value())
        settings.setValue("level_2", self.level2Value.value())
        settings.setValue("open_start", int(self.open.freq[0]))
        settings.setValue("open_stop", int(self.open.freq[-1]))
        settings.setValue("open_size", self.open.freq.size)
        settings.setValue("short_start", int(self.short.freq[0]))
        settings.setValue("short_stop", int(self.short.freq[-1]))
        settings.setValue("short_size", self.short.freq.size)
        settings.setValue("load_start", int(self.load.freq[0]))
        settings.setValue("load_stop", int(self.load.freq[-1]))
        settings.setValue("load_size", self.load.freq.size)
        settings.setValue("dut_start", int(self.dut.freq[0]))
        settings.setValue("dut_stop", int(self.dut.freq[-1]))
        settings.setValue("dut_size", self.dut.freq.size)
        for i in range(len(FigureTab.cursors)):
            settings.setValue("cursor_%d" % i, FigureTab.cursors[i])
        data = self.open.data
        for i in range(self.open.freq.size):
            settings.setValue("open_real_%d" % i, float(data.real[i]))
            settings.setValue("open_imag_%d" % i, float(data.imag[i]))
        data = self.short.data
        for i in range(self.short.freq.size):
            settings.setValue("short_real_%d" % i, float(data.real[i]))
            settings.setValue("short_imag_%d" % i, float(data.imag[i]))
        data = self.load.data
        for i in range(self.load.freq.size):
            settings.setValue("load_real_%d" % i, float(data.real[i]))
            settings.setValue("load_imag_%d" % i, float(data.imag[i]))
        data = self.dut.data
        for i in range(self.dut.freq.size):
            settings.setValue("dut_real_%d" % i, float(data.real[i]))
            settings.setValue("dut_imag_%d" % i, float(data.imag[i]))

    def read_cfg_settings(self, settings):
        self.addrValue.setText(settings.value("addr", "192.168.1.100"))
        self.rateValue.setCurrentIndex(settings.value("rate", 0, type=int))
        self.corrValue.setValue(settings.value("corr", 0, type=int))
        self.phase1Value.setValue(settings.value("phase_1", 0, type=int))
        self.phase2Value.setValue(settings.value("phase_2", 0, type=int))
        self.level1Value.setValue(settings.value("level_1", 0, type=int))
        self.level2Value.setValue(settings.value("level_2", -90, type=int))
        open_start = settings.value("open_start", 10, type=int)
        open_stop = settings.value("open_stop", 50000, type=int)
        open_size = settings.value("open_size", 5000, type=int)
        short_start = settings.value("short_start", 10, type=int)
        short_stop = settings.value("short_stop", 50000, type=int)
        short_size = settings.value("short_size", 5000, type=int)
        load_start = settings.value("load_start", 10, type=int)
        load_stop = settings.value("load_stop", 50000, type=int)
        load_size = settings.value("load_size", 5000, type=int)
        dut_start = settings.value("dut_start", 10, type=int)
        dut_stop = settings.value("dut_stop", 50000, type=int)
        dut_size = settings.value("dut_size", 5000, type=int)
        self.startValue.setValue(dut_start)
        self.stopValue.setValue(dut_stop)
        self.sizeValue.setValue(dut_size)
        for i in range(len(FigureTab.cursors)):
            FigureTab.cursors[i] = settings.value("cursor_%d" % i, FigureTab.cursors[i], type=int)
        self.open.freq = np.linspace(open_start, open_stop, open_size)
        self.open.data = np.zeros(open_size, np.complex64)
        for i in range(open_size):
            real = settings.value("open_real_%d" % i, 0.0, type=float)
            imag = settings.value("open_imag_%d" % i, 0.0, type=float)
            self.open.data[i] = real + 1.0j * imag
        self.short.freq = np.linspace(short_start, short_stop, short_size)
        self.short.data = np.zeros(short_size, np.complex64)
        for i in range(short_size):
            real = settings.value("short_real_%d" % i, 0.0, type=float)
            imag = settings.value("short_imag_%d" % i, 0.0, type=float)
            self.short.data[i] = real + 1.0j * imag
        self.load.freq = np.linspace(load_start, load_stop, load_size)
        self.load.data = np.zeros(load_size, np.complex64)
        for i in range(load_size):
            real = settings.value("load_real_%d" % i, 0.0, type=float)
            imag = settings.value("load_imag_%d" % i, 0.0, type=float)
            self.load.data[i] = real + 1.0j * imag
        self.dut.freq = np.linspace(dut_start, dut_stop, dut_size)
        self.dut.data = np.zeros(dut_size, np.complex64)
        for i in range(dut_size):
            real = settings.value("dut_real_%d" % i, 0.0, type=float)
            imag = settings.value("dut_imag_%d" % i, 0.0, type=float)
            self.dut.data[i] = real + 1.0j * imag

    def write_csv(self):
        dialog = QFileDialog(self, "Write csv file", ".", "*.csv")
        dialog.setDefaultSuffix("csv")
        dialog.setAcceptMode(QFileDialog.AcceptSave)
        dialog.setOptions(QFileDialog.DontConfirmOverwrite)
        if dialog.exec() == QDialog.Accepted:
            name = dialog.selectedFiles()
            fh = open(name[0], "w")
            f = self.dut.freq
            o = self.interp(f, self.open)
            s = self.interp(f, self.short)
            l = self.interp(f, self.load)
            d = self.dut.data
            fh.write("frequency;open.real;open.imag;short.real;short.imag;load.real;load.imag;dut.real;dut.imag\n")
            for i in range(f.size):
                fh.write("0.0%.8d;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f;%12.9f\n" % (f[i] * 1000, o.real[i], o.imag[i], s.real[i], s.imag[i], l.real[i], l.imag[i], d.real[i], d.imag[i]))
            fh.close()

    def write_s1p(self):
        dialog = QFileDialog(self, "Write s1p file", ".", "*.s1p")
        dialog.setDefaultSuffix("s1p")
        dialog.setAcceptMode(QFileDialog.AcceptSave)
        dialog.setOptions(QFileDialog.DontConfirmOverwrite)
        if dialog.exec() == QDialog.Accepted:
            name = dialog.selectedFiles()
            fh = open(name[0], "w")
            freq = self.dut.freq
            gamma = self.gamma(freq)
            fh.write("# GHz S MA R 50\n")
            for i in range(freq.size):
                fh.write("0.0%.8d   %8.6f %7.2f\n" % (freq[i] * 1000, np.absolute(gamma[i]), np.angle(gamma[i], deg=True)))
            fh.close()

    def write_s2p(self, gain):
        dialog = QFileDialog(self, "Write s2p file", ".", "*.s2p")
        dialog.setDefaultSuffix("s2p")
        dialog.setAcceptMode(QFileDialog.AcceptSave)
        dialog.setOptions(QFileDialog.DontConfirmOverwrite)
        if dialog.exec() == QDialog.Accepted:
            name = dialog.selectedFiles()
            fh = open(name[0], "w")
            freq = self.dut.freq
            gamma = self.gamma(freq)
            fh.write("# GHz S MA R 50\n")
            for i in range(freq.size):
                fh.write(
                    "0.0%.8d   %8.6f %7.2f   %8.6f %7.2f   0.000000    0.00   0.000000    0.00\n"
                    % (freq[i] * 1000, np.absolute(gamma[i]), np.angle(gamma[i], deg=True), np.absolute(gain[i]), np.angle(gain[i], deg=True))
                )
            fh.close()

    def write_s2p_short(self):
        self.write_s2p(self.gain_short(self.dut.freq))

    def write_s2p_open(self):
        self.write_s2p(self.gain_open(self.dut.freq))


warnings.filterwarnings("ignore")
app = QApplication(sys.argv)
dpi = app.primaryScreen().logicalDotsPerInch()
matplotlib.rcParams["figure.dpi"] = dpi
window = VNA()
window.update_tab()
window.show()
sys.exit(app.exec_())
