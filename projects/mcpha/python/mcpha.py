#!/usr/bin/env python

# Control program for the Red Pitaya Multichannel Pulse Height Analyzer
# Copyright (C) 2015  Pavel Demin
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

from PyQt5.uic import loadUiType
from PyQt5.QtCore import QRegExp, QTimer
from PyQt5.QtGui import QRegExpValidator, QPalette, QColor
from PyQt5.QtWidgets import QApplication, QMainWindow, QMenu, QVBoxLayout, QSizePolicy, QMessageBox, QWidget
from PyQt5.QtNetwork import QAbstractSocket, QTcpSocket

Ui_MCPHA, QMainWindow = loadUiType('mcpha.ui')
Ui_MCPHAHist, QWidget = loadUiType('mcpha_hist.ui')
Ui_MCPHAScope, QWidget = loadUiType('mcpha_scope.ui')

class MCPHA(QMainWindow, Ui_MCPHA):
  rates = {0:25.0e3, 1:50.0e3, 2:250.0e3, 3:500.0e3, 4:2500.0e3}
  def __init__(self):
    super(MCPHA, self).__init__()
    self.setupUi(self)
    # IP address validator
    rx = QRegExp('^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$')
    self.addrValue.setValidator(QRegExpValidator(rx, self.addrValue))
    # create TCP socket
    self.socket = QTcpSocket(self)
    #self.socket.connected.connect(self.connected)
    #self.socket.readyRead.connect(self.read_data)
    #self.socket.error.connect(self.display_error)

class MCPHAHist(QWidget, Ui_MCPHAHist):
  def __init__(self):
    super(MCPHAHist, self).__init__()
    self.setupUi(self)

class MCPHAScope(QWidget, Ui_MCPHAScope):
  def __init__(self):
    super(MCPHAScope, self).__init__()
    self.setupUi(self)
    palette = QPalette(self.ch1Label.palette())
    palette.setColor(QPalette.Background, QColor('yellow'))
    palette.setColor(QPalette.Foreground, QColor('black'))
    self.ch1Label.setAutoFillBackground(True)
    self.ch1Label.setPalette(palette)
    self.ch1Value.setAutoFillBackground(True)
    self.ch1Value.setPalette(palette)
    palette.setColor(QPalette.Background, QColor('cyan'))
    palette.setColor(QPalette.Foreground, QColor('black'))
    self.ch2Label.setAutoFillBackground(True)
    self.ch2Label.setPalette(palette)
    self.ch2Value.setAutoFillBackground(True)
    self.ch2Value.setPalette(palette)

app = QApplication(sys.argv)
window = MCPHA()
window.show()
hist1 = MCPHAHist()
hist2 = MCPHAHist()
scope = MCPHAScope()
window.hist1Layout.addWidget(hist1)
window.hist2Layout.addWidget(hist2)
window.scopeLayout.addWidget(scope)
sys.exit(app.exec_())
