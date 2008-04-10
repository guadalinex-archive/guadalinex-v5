# -*- coding: utf-8 -*-
# Copyright (C) 2006-2007  Vodafone España, S.A.
# Author:  Pablo Martí
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
Stuff used at startup
"""
__version__ = "$Rev: 1172 $"

import os
import re

from serial.serialutil import SerialException

import gnome

from twisted.application.service import Service
from twisted.internet.task import LoopingCall
from twisted.python import log

from vmc.contrib.louie import install_plugin, plugin
install_plugin(plugin.TwistedDispatchPlugin())

from vmc.common.consts import APP_LONG_NAME
from vmc.common.config import config
from vmc.common.configbase import DeviceProfileCache
from vmc.common.encoding import _
import vmc.common.exceptions as ex
from vmc.common.hardware import DeviceListener, HardwareRegistry
import vmc.common.notifications as N
from vmc.common.plugin import DBusDevicePlugin
from vmc.common.startup import populate_dbs


from vmc.gtk.controllers.initialconf import (NewProfileController,
                                             DeviceSelectionController)
from vmc.gtk.controllers.splash import SplashController
from vmc.gtk import Model
from vmc.gtk.models.initialconf import NewProfileModel
from vmc.gtk.views.initialconf import NewProfileView, DeviceSelectionView
from vmc.gtk.views.splash import SplashView

class GTKSerialService(Service):
    """
    I am a twistd Service that starts a GUI and hooks up a serial port
    """
    
    def __init__(self):
        self.ctrl = None
    
    def startService(self):
        Service.startService(self)
        self.ctrl = GTKStartupController()
    
    def get_sconn(self):
        return self.ctrl.device.sconn
    
class SplashContainer(object):
    """
    I act as a Container for all the splash stuff
    """
    def __init__(self):
        self.ctrl = SplashController(Model())
        self.view = SplashView(self.ctrl)
        self.loop = None
        self.view.show()
    
    def pulse(self):
        self.view['splash_progress_bar'].pulse()
    
    def start_pulse(self):
        if not self.loop:
            self.loop = LoopingCall(self.pulse)
            self.loop.start(.2, now=True)
        else:
            raise Exception("Can't start loop twice")
    
    def stop_pulse(self):
        if self.loop:
            self.loop.stop()
            self.loop = None
        else:
            raise Exception("Can't stop a loop that hasn't started")
    
    def set_fraction(self, fraction):
        """Sets the given fraction in the progress bar"""
        try:
            self.view['splash_progress_bar'].set_fraction(fraction)
        except:
            pass
        
    def set_text(self, text):
        """Sets the given text in the progress bar"""
        try:
            self.view['splash_progress_bar'].set_text(text)
        except:
            pass
    
    def close(self):
        self.ctrl.model.unregister_observer(self.ctrl)
        self.view.get_top_widget().destroy()
        self.ctrl = None
        self.view = None
    

class GTKStartupController(object):
    """
    I control the startup process, basically I maintain a counter with
    the number of common.exception.SIMFailureError exceptions received
    
    If that number is greater than 3 that means that there is no SIM
    inserted in the 3G device. I exist because there are some 3G devices
    that never inform you that there's no SIM inserted, they will just
    whine about a C{common.exceptions.SIMFailureError}
    """
    def __init__(self):
        self.device = None
        self.splash = None
        self.num_sim_failure_errors = 0
        self.init()

    def init(self):
        """I start the application"""
        # avoid gnome warnings
        gnome.init(APP_LONG_NAME, __version__)
        
        self.splash = SplashContainer()
        self.splash.pulse()
        
        profile_name = config.get('profile', 'name')
        
        if not profile_name: # user never never ran the app
            self.splash.set_text(_('Initial setup...'))
            self.splash.pulse()
            # populate databases and configure hardware afterwards
            populate_dbs()
        
        self.splash.pulse()
        self.detect_hardware()
         
    def detect_hardware(self, ignored=None):
        
        def _ask_user_for_device(devices, callback, splash):
            cached_devices = DeviceProfileCache.get_cached_devices()

            for cached_device in list(cached_devices):
                if isinstance(cached_device, DBusDevicePlugin) \
                        and not cached_device in devices:
                    cached_devices.remove(cached_device)
                    
            all_devices = cached_devices + [device for device in devices
                                            if device not in cached_devices]
            controller = DeviceSelectionController(Model(),
                                                all_devices, callback, splash)
            view = DeviceSelectionView(controller)
            view.show()

        def _device_select(devices, callback, splash):
            try:
                last_device_id = config.get_last_device()
                device = DeviceProfileCache.load(last_device_id)
            
                #If device is a DBus device, it has to have been detected to be
                #used.
                if (isinstance(device, DBusDevicePlugin) and 
                    not device in devices):
                    _ask_user_for_device(devices, callback, splash)
                else:
                    callback(device)
            except IOError:
                _ask_user_for_device(devices, callback, splash)
                
        def device_serial_eb(failure):
            from vmc.gtk import dialogs
            failure.trap(SerialException)
            message = _('Device setup not completed')
            details = _("""
Your device has been detected but it has been impossible to connect to it.

%s""") % failure.getErrorMessage()
            dialogs.open_warning_dialog(message, details)  
            _device_select([], self.configure_hardware, self.splash)

        def device_not_found_eb(failure):
            failure.trap(ex.DeviceNotFoundError)       
            _device_select([], self.configure_hardware, self.splash)

        def device_lacks_extractinfo_eb(failure):
            failure.trap(ex.DeviceLacksExtractInfo)
            from vmc.gtk import dialogs
            from vmc.common.shutdown import shutdown_core
            
            device = failure.value.args[0]
            info = dict(name=device.name, vmc=APP_LONG_NAME)
            message = _('Device setup not completed')
            details = _("""
Your device "%(name)s" is not properly registered with the kernel. %(vmc)s
needs at least two serial ports to communicate with your %(name)s.
The program includes a set of udev rules plus some binaries that make sure
that your device is always recognised properly. If you've installed from
source, then make sure to copy the relevant files in the contrib dir
""") % info
            dialogs.open_warning_dialog(message, details)
            
            shutdown_core(delay=.2)
        
        def device_timeout_eb(failure):
            failure.trap(ex.ATTimeout)
            from vmc.gtk import dialogs
            from vmc.common.shutdown import shutdown_core
            
            message = _('Device not responding')
            details = _("""
Your device took more than 15 seconds to reply to my last command. Unplug it,
plug it again, and try in a moment.""")
            dialogs.open_warning_dialog(message, details)
            shutdown_core(delay=.2)
        
        def get_devices_cb(devices):
            _device_select(devices, self.configure_hardware, self.splash)
        
        hw_reg = HardwareRegistry()
        d = hw_reg.get_devices()
        d.addCallback(get_devices_cb)
        d.addErrback(device_not_found_eb)
        d.addErrback(device_lacks_extractinfo_eb)
        d.addErrback(device_timeout_eb)
        d.addErrback(device_serial_eb)
        d.addErrback(log.err)
    
    def configure_hardware(self, device):
        from vmc.common.hardware.base import DeviceResolver
        def identify_dev_cb(model):
            self.device = device

            DeviceProfileCache.store(device)
            config.set_last_device(device.cached_id)

            hook_it_up(self.splash, DeviceListener(self.device), self.device)

        def identify_dev_eb(failure):
            failure.trap(SerialException)
            
            from vmc.gtk import dialogs
            info = dict(name=device.name, vmc=APP_LONG_NAME)
            message = _('Device setup not completed')
            details = _("""
%(vmc)s cannot connect with the selected device (%(name)s).
""") % info
            dialogs.open_warning_dialog(message, details)
            config.set_last_device('')
            self.detect_hardware()

        d = DeviceResolver.identify_device(device)
        d.addCallback(identify_dev_cb)
        d.addErrback(identify_dev_eb)


def hook_it_up(splash, device_listener, device=None):
    """Attachs comms core to GUI and presents main screen"""
    # get main screen up
    from vmc.gtk.models.application import ApplicationModel
    from vmc.gtk.views.application import ApplicationView
    from vmc.gtk.controllers.application import ApplicationController
    
    splash.pulse()
    
    model = ApplicationModel()
    ctrl = ApplicationController(model, device_listener, splash)
    view = ApplicationView(ctrl)
    # we keep a reference of the controller in the model
    model.ctrl = ctrl
    
    if not device:
        ctrl.start()
        return
    
    unsolicited_notifications_callbacks = {
        N.SIG_RSSI : ctrl._change_signal_level,
        N.SIG_SPEED : ctrl._change_net_stats_cb,
        N.SIG_NEW_CONN_MODE : ctrl._conn_mode_changed,
        N.SIG_SMS : ctrl._on_sms_received,
        N.SIG_CALL : None,
    }
    
    profile_name = config.get('profile', 'name')
    
    statemachine_callbacks = {}
    
    if not profile_name:
        # user never run the app before
        def configure_device():
            _model = NewProfileModel(device)
            _ctrl = NewProfileController(_model, startup=True, aux_ctrl=ctrl)
            _view = NewProfileView(_ctrl)
            _view.set_parent_view(view) # center on main screen
            _view.show()
        
        statemachine_callbacks['PostInitExit'] = configure_device
    else:
        statemachine_callbacks['PostInitExit'] = ctrl.start
    
    splash.start_pulse()
    
    def on_auth_exit():
        splash.set_text(_('Authenticated!'))
        splash.stop_pulse()
        
    statemachine_callbacks['PreInitExit'] = lambda: splash.set_text(_('Authenticating...'))
    statemachine_callbacks['AuthExit'] = on_auth_exit
    
    statemachine_errbacks = {
        'AlreadyConnecting' : None,
        'AlreadyConnected' : None,
        'IllegalOperationError' : ctrl.on_illegal_operation,
    }
    
    from vmc.gtk.wrapper import GTKWrapper
    ctrl.model.wrapper = GTKWrapper(device,
                                    unsolicited_notifications_callbacks,
                                    statemachine_callbacks,
                                    statemachine_errbacks, ctrl)
    
    ctrl.model.wrapper.start_behaviour(ctrl)

def check_posix_dependencies():
    """
    Returns a list with all the import's problems
    
    This is run at startup to ease users solving dependencies problems
    """
    resp = []
    try:
        import pygtk
        pygtk.require("2.0")
    except ImportError:
        resp.append("python-gtk2 module not found, please install it")
    except AssertionError:
        resp.append("python-gtk module found, please upgrade to python-gtk2")
    
    try:
        from twisted.copyright import version
        if [int(x) for x in re.search(r'^(\d+)\.(\d+)\.(\d+)',
                      version).groups()] < [ 2, 2, 0, ]:
            resp.append("python-twisted module is too old, please upgrade it")
    
    except ImportError:
        resp.append("python-twisted module not found, please install it")
    
    import gtk
    if not hasattr(gtk, 'StatusIcon'):
        try:
            import egg.trayicon
        except ImportError:
            resp.append("egg.trayicon module not found, please install it")
    
    return resp

def check_win_dependencies():
    # XXX: Win32 here
    return []

if os.name == 'posix':
    check_dependencies = check_posix_dependencies
elif os.name == 'nt':
    check_dependencies = check_win_dependencies
