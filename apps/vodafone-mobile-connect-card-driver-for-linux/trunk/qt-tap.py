# -*- coding: utf-8 -*-
# Copyright (C) 2006-2007  Vodafone España, S.A.
# Author:  Pablo Martí & Rafael Treviño
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
"""
Starts Vodafone Mobile Connect driver for Linux

execute it with:  twistd -r qt4 -noy qt-tap.py
"""

import locale
import signal
import sys

from twisted.application.service import Application, IProcess
from twisted.application import strports

__version__ = "$Rev: 1172 $"

# i10n stuff
locale.setlocale(locale.LC_ALL, '')

from vmc.common.startup import create_skeleton_and_do_initial_setup

# it will just return if its not necessary
create_skeleton_and_do_initial_setup()

# we delays this imports as they depend on modules that in turn depend on
# stuff that depend on plugins. If we dont delay this imports after the
# initial setup is complete we would get a messy traceback
from vmc.qt.startup import check_dependencies, QtSerialService
from vmc.common import shell
from vmc.common.consts import APP_LONG_NAME, APP_SHORT_NAME, SSH_PORT
from PyQt4.QtGui import QMessageBox, QApplication
from vmc.common.encoding import _

# TODO: Think it!
# app = QApplication(sys.argv)

probs = check_dependencies()
if probs:
    message = _('Missing dependencies')
    probtext = '\n'.join(probs)
    details = _('The following dependencies are not satisfied:\n%s') % probtext
    QMessageBox.warning(None, message, details)
    raise SystemExit()

# access osobj singleton
from vmc.common.exceptions import OSNotRegisteredError
try:
    from vmc.common.oal import osobj
except OSNotRegisteredError:
    message = _('OS/Distro not registered')
    details = """
The OS/Distro under which you are running %s
is not registered in the OS database. Check the common.oal module for what
you can do in order to support your OS/Distro
""" % APP_LONG_NAME
    QMessageBox.warning(None, message, details)
    raise SystemExit()

if osobj.manage_secrets:
    probs = osobj.check_permissions()
    if probs:
        message = _('Permissions problems')
        probtext = '\n'.join(probs)
        details = """
%s needs the following files and dirs with some specific permissions
in order to work properly:\n%s""" % (APP_LONG_NAME, probtext)
        QMessageBox.warning(None, message, details)
        raise SystemExit()

from vmc.common.shutdown import shutdown_core
signal.signal(signal.SIGINT, shutdown_core)

service = QtSerialService()
application = Application(APP_SHORT_NAME)
factory = shell.getManholeFactory(globals(), admin='admin')
strports.service(SSH_PORT, factory).setServiceParent(application)
IProcess(application).processName = APP_SHORT_NAME
service.setServiceParent(application)
