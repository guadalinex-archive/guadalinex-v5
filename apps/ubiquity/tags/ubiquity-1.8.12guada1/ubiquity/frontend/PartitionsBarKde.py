# -*- coding: utf-8 -*-
#
# Copyright (C) 2006 Canonical Ltd.
#
# Author:
#   Jonathan Riddell <jriddell@ubuntu.com>
#
# This file is part of Ubiquity.
#
# Ubiquity is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or at your option)
# any later version.
#
# Ubiquity is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with Ubiquity; if not, write to the Free Software Foundation, Inc., 51
# Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
##################################################################################

import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import uic

class PartitionFrame(QFrame):
    def mouseReleaseEvent(self, event):
        self.emit(SIGNAL("clicked()"))
        self.setFrameShadow(QFrame.Sunken)

class Partition:

    filesystemColours = {'ext3': Qt.darkCyan,
                         'free': Qt.yellow,
                         'linux-swap': Qt.cyan,
                         'fat32': Qt.green,
                         'fat16': Qt.green,
                         'ntfs': Qt.magenta}

    def __init__(self, size, index, fs, path, parent):
        self.size = size
        self.frame = PartitionFrame(parent)
        self.frame.setLineWidth(1)
        self.frame.setFrameShadow(QFrame.Raised)
        self.frame.setFrameShape(QFrame.Box)
        sizePolicy = self.frame.sizePolicy()
        sizePolicy.setHorizontalStretch(size)
        self.frame.setSizePolicy(sizePolicy)
        QApplication.instance().connect(self.frame, SIGNAL("clicked()"), self.clicked)
        self.fs = fs
        self.path = path

        layout = QHBoxLayout(self.frame)
        label = QLabel(path, self.frame)
        layout.addWidget(label)

        self.frame.setAutoFillBackground(True)
        palette = self.frame.palette()
        try:
          palette.setColor(QPalette.Active, QPalette.Background, self.filesystemColours[fs])
          palette.setColor(QPalette.Inactive, QPalette.Background, self.filesystemColours[fs])
          """ #FIXME doesn't do anything
          colour = QColor(self.filesystemColours[fs])
          red = 256 - colour.red()
          green = 256 - colour.green()
          blue = 256 - colour.blue()
          inverseColour = QColor(red, green, blue)
          palette.setColor(QPalette.Normal, QPalette.Text, inverseColour)
          label.setPalette(palette)
          """
          self.frame.setPalette(palette)
        except KeyError:
          pass

        self.index = index

        parent.layout.addWidget(self.frame)
        self.parent = parent

    def clicked(self):
        self.parent.clicked(self.index)

    def raiseFrames(self):
        self.frame.setFrameShadow(QFrame.Raised)

class PartitionsBar(QWidget):
    """ a widget to graphically show disk partitions.  Made of a row of QFrames. """
    def __init__(self, diskSize, parent = None):
        QWidget.__init__(self, parent)
        self.layout = QHBoxLayout()
        self.layout.setMargin(2)
        self.layout.setSpacing(0)
        self.setLayout(self.layout)
        self.partitions = []
        self.diskSize = diskSize
        self.setMinimumHeight(30)
        sizePolicy = self.sizePolicy()
        sizePolicy.setVerticalStretch(10)
        sizePolicy.setVerticalPolicy(QSizePolicy.Fixed)
        self.setSizePolicy(sizePolicy)

    def addPartition(self, size, index, fs, path):
        partition = Partition(size, index, fs, path, self)
        self.partitions.append(partition)

    def clicked(self, index):
        self.emit(SIGNAL("clicked(int)"), index)

    def raiseFrames(self):
        for partition in self.partitions:
            partition.frame.setFrameShadow(QFrame.Raised)

    def selected(self, index):
        for partition in partitions:
            if partition.index == index:
                partition.clicked()
