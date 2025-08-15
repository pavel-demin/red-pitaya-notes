from py2exe import freeze

includes = [
    "PySide6.QtCore",
    "PySide6.QtGui",
    "PySide6.QtNetwork",
    "PySide6.QtMultimedia",
    "PySide6.QtPrintSupport",
    "PySide6.QtUiTools",
    "PySide6.QtWebSockets",
    "PySide6.QtWidgets",
    "PySide6.QtXml",
    "numpy",
    "matplotlib.backends.backend_qtagg",
]

freeze(
    windows=[{"script": "exec.py"}],
    data_files=[
        (
            "",
            [
                "c:\\Python313\\Lib\\site-packages\\PySide6\\Qt6OpenGL.dll",
                "c:\\Python313\\Lib\\site-packages\\PySide6\\Qt6Designer.dll",
                "c:\\Python313\\Lib\\site-packages\\PySide6\\Qt6DesignerComponents.dll",
                "c:\\Python313\\Lib\\site-packages\\PySide6\\designer.exe",
                "c:\\Python313\\Lib\\site-packages\\PySide6\\uic.exe",
            ],
        ),
        (
            "platforms",
            [
                "c:\\Python313\\Lib\\site-packages\\PySide6\\plugins\\platforms\\qwindows.dll"
            ],
        ),
        (
            "styles",
            [
                "c:\\Python313\\Lib\\site-packages\\PySide6\\plugins\\styles\\qmodernwindowsstyle.dll"
            ],
        ),
    ],
    options={"includes": includes, "bundle_files": 3, "compressed": True},
)
