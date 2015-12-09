
import sys

import numpy as np

import matplotlib
matplotlib.use("Qt5Agg")

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp
from PyQt5.QtGui import QRegExpValidator
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget

from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg as FigureCanvas
from matplotlib.figure import Figure

Ui_PulsedNMR, QMainWindow = loadUiType('pulsed_nmr.ui')

class PulsedNMR(QMainWindow, Ui_PulsedNMR):
  def __init__(self):
    super(PulsedNMR, self).__init__()
    self.setupUi(self)
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5]).){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    fig = Figure()
    fig.set_facecolor('none')
    self.axes = fig.add_subplot(111)
    self.axes.grid()
    self.canvas = FigureCanvas(fig)
    self.plot.addWidget(self.canvas)
    self.canvas.draw()
        
app = QApplication(sys.argv)
window = PulsedNMR()
window.show()
sys.exit(app.exec_())
