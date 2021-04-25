from setuptools import setup
import os
import py2exe
import matplotlib

includes = [
  'PySide2.QtCore',
  'PySide2.QtGui',
  'PySide2.QtNetwork',
  'PySide2.QtMultimedia',
  'PySide2.QtPrintSupport',
  'PySide2.QtUiTools',
  'PySide2.QtWebSockets',
  'PySide2.QtWidgets',
  'PySide2.QtXml',
  'numpy',
  'matplotlib.backends.backend_qt5agg'
]

setup(
  windows = [{'script': 'exec.py'}],
  data_files = [
    ('', ['c:\\Python38\\Scripts\\pyside2-uic.exe', 'c:\\Python38\\Lib\\site-packages\\PySide2\\Qt5Designer.dll', 'c:\\Python38\\Lib\\site-packages\\PySide2\\Qt5DesignerComponents.dll', 'c:\\Python38\\Lib\\site-packages\\PySide2\\designer.exe']),
    ('platforms', ['c:\\Python38\\Lib\\site-packages\\PySide2\\plugins\\platforms\\qwindows.dll']),
    ('styles', ['c:\\Python38\\Lib\\site-packages\\PySide2\\plugins\\styles\\qwindowsvistastyle.dll'])
  ],
  options = {
    'py2exe':{
      'includes': includes,
      'bundle_files': 3,
      'compressed': True
    }
  }
)
