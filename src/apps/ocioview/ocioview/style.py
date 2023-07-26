# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

from PySide2 import QtGui


# Application style sheet overrides
QSS = """
"""


class DarkPalette(QtGui.QPalette):
    def __init__(self):
        super().__init__()

        # Central roles
        self.setColor(QtGui.QPalette.Window, QtGui.QColor(60, 60, 60))
        self.setColor(QtGui.QPalette.WindowText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.Base, QtGui.QColor(50, 50, 50))
        self.setColor(QtGui.QPalette.AlternateBase, QtGui.QColor(45, 45, 45))
        self.setColor(QtGui.QPalette.ToolTipBase, QtGui.QColor(45, 45, 45))
        self.setColor(QtGui.QPalette.ToolTipText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.PlaceholderText, QtGui.QColor(100, 100, 100))
        self.setColor(QtGui.QPalette.Text, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.Button, QtGui.QColor(50, 50, 50))
        self.setColor(QtGui.QPalette.ButtonText, QtGui.QColor(190, 190, 190))
        self.setColor(QtGui.QPalette.BrightText, QtGui.QColor(190, 190, 190))

        # 3D effects
        self.setColor(QtGui.QPalette.Dark, QtGui.QColor(47, 47, 47))
        self.setColor(QtGui.QPalette.Mid, QtGui.QColor(64, 64, 64))
        self.setColor(QtGui.QPalette.Midlight, QtGui.QColor(81, 81, 81))

        # Highlight
        self.setColor(QtGui.QPalette.Highlight, QtGui.QColor(204, 30, 104))
        self.setColor(QtGui.QPalette.HighlightedText, QtGui.QColor(190, 190, 190))

        # Disabled
        disabled = QtGui.QPalette.Disabled
        self.setColor(disabled, QtGui.QPalette.Base, QtGui.QColor(57, 57, 57))
        self.setColor(disabled, QtGui.QPalette.Text, QtGui.QColor(100, 100, 100))
        self.setColor(disabled, QtGui.QPalette.Button, QtGui.QColor(57, 57, 57))
        self.setColor(disabled, QtGui.QPalette.ButtonText, QtGui.QColor(115, 115, 115))
