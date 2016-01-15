from setuptools import setup
import os
import py2exe
import matplotlib

includes = [
  'sip',
  'yaml',
  'PyQt5',
  'PyQt5.uic',
  'PyQt5.QtCore',
  'PyQt5.QtDesigner',
  'PyQt5.QtGui',
  'PyQt5.QtNetwork',
  'PyQt5.QtMultimedia',
  'PyQt5.QtPrintSupport',
  'PyQt5.QtWebSockets',
  'PyQt5.QtWidgets',
  'PyQt5.QtXml',
  'numpy',
  'matplotlib.backends.backend_qt5agg'
]

setup(
  windows = [{'script': 'exec.py'}],
  data_files = matplotlib.get_py2exe_datafiles() + [
    ('', ['d:\\Python34\\Lib\\site-packages\\PyQt5\\Qt5DesignerComponents.dll', 'd:\\Python34\\Lib\\site-packages\\PyQt5\\designer.exe']),
    ('platforms', ['d:\\Python34\\Lib\\site-packages\\PyQt5\\plugins\\platforms\\qwindows.dll'])
  ],
  options = {
    'py2exe':{
      'includes': includes,
      'bundle_files': 2,
      'compressed': True
    }
  }
)
