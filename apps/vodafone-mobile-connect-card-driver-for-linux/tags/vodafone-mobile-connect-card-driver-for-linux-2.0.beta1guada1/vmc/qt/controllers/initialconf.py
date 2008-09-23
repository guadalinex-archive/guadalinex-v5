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
Controller for the initial configuration window
"""
__version__ = "$Rev: 1172 $"

from twisted.internet import threads, reactor

from serial.serialutil import SerialException

from vmc.common.configbase import DeviceProfileCache
from vmc.common.dialers import AUTH_OPTS_DICT, AUTH_OPTS_DICT_REV
from vmc.common.encoding import _
from vmc.common.hardware import CONN_OPTS_DICT, CONN_OPTS_DICT_REV
from vmc.common.hardware.base import DeviceResolver
from vmc.common.hardware._dbus import DbusComponent
from vmc.common.profiles import get_profile_manager
from vmc.common.plugin import PluginManager
from vmc.common.shutdown import shutdown_core
from vmc.common.interfaces import IDBusDevicePlugin
from vmc.gtk import Controller
import vmc.gtk.dialogs as dialogs
from vmc.contrib.ValidatedEntry import ValidatedEntry, v_ip

INVALID_CHARS = ['/', '\\']


class BaseProfileController(Controller):
    def __init__(self, model, quiet=False):
        super(BaseProfileController, self).__init__(model)
        self.suggested_name = None
        self.quiet = quiet
        # XXX: One day this could live in the glade file, we need to add
        # it as a custom widget to glade3
        self.dns1_entry = ValidatedEntry(v_ip)
        self.dns2_entry = ValidatedEntry(v_ip)
    
    def register_view(self, view):
        super(BaseProfileController, self).register_view(view)
        # prepare everythin
        self.view['vbox3'].set_sensitive(False)
        # now attach the dns entries to the vbox
        vbox = self.view['vbox3']
        vbox.pack_start(self.dns1_entry)
        vbox.pack_start(self.dns2_entry)
        # get device model
        name = self.model.get_device().name
        self.view['device_model_label'].set_text(name)
        
        if not self.quiet:
            self.try_to_load_profile_from_imsi_prefix()
    
    def propose_profile_name(self):
        self.suggested_name = '-'.join([self.model.get_device().name,
                        self.get_connection_combobox_opt()]).replace(' ', '')
        self.view['profile_name_entry'].set_text(self.suggested_name)
    
    def is_profile_name_insane(self):
        """
        Returns True if the profile name has invalid characters
        """
        text = self.view['profile_name_entry'].get_text()
        for char in INVALID_CHARS:
            if char in text:
                return True
        
        return False
    
    def try_to_load_profile_from_imsi_prefix(self):
        def get_profile_cb(profile):
            if profile:
                message = _('Configuration details found for Operator!')
                details = _(
"""Do you want to load stored settings for %s?""") % profile.get_full_name()
                resp = dialogs.open_confirm_action_dialog(_('Load'),
                                                          message, details)
                if resp:
                    self.load_network_profile(profile)
                    self.propose_profile_name()
        
        self.model.get_profile_from_imsi_prefix().addCallback(get_profile_cb)
    
    def load_mobile_profile(self, profile):
        CON = 'connection'
        
        self.view['profile_name_entry'].set_text(profile.name)
        self.view['username_entry'].set_text(profile.get(CON, 'username'))
        self.view['password_entry'].set_text(profile.get(CON, 'username'))
        self.view['apn_entry'].set_text(profile.get(CON, 'apn'))
        if profile.get(CON, 'dns1') or profile.get(CON, 'dns2'):
            self.view['dns_checkbutton'].set_active(True)
            self.dns1_entry.set_text(profile.get(CON, 'dns1'))
            self.dns2_entry.set_text(profile.get(CON, 'dns2'))
        
        # now set auth and combobox
        self.set_auth_combobox_opt(profile.get(CON, 'dialer_profile'))
        self.set_connection_combobox_opt(profile.get(CON, CON))
    
    def load_network_profile(self, profile):
        self.view['username_entry'].set_text(profile.username)
        self.view['password_entry'].set_text(profile.password)
        self.view['apn_entry'].set_text(profile.apn)
        if profile.dns1 or profile.dns2:
            self.view['dns_checkbutton'].set_active(True)
            self.dns1_entry.set_text(profile.dns1)
            self.dns2_entry.set_text(profile.dns2)
    
    def get_auth_combobox_opt(self):
        model = self.view['auth_combobox'].get_model()
        index = self.view['auth_combobox'].get_active()
        if index < 0:
            # no auth selected on combobox
            return None
        else:
            return unicode(model[index][0], 'utf8')
    
    def set_auth_combobox_opt(self, opt):
        model = self.view['auth_combobox'].get_model()
        for i, row in enumerate(model):
            if row[0] == AUTH_OPTS_DICT_REV[opt]:
                self.view['auth_combobox'].set_active(i)
                break
    
    def get_connection_combobox_opt(self):
        model = self.view['connection_combobox'].get_model()
        index = self.view['connection_combobox'].get_active()
        if index < 0:
            # no connection selected on combobox
            return None
        else:
            return unicode(model[index][0], 'utf8')
    
    def set_connection_combobox_opt(self, opt):
        model = self.view['connection_combobox'].get_model()
        for i, row in enumerate(model):
            if row[0] == CONN_OPTS_DICT_REV[opt]:
                self.view['connection_combobox'].set_active(i)
                break
    
    def get_profile_settings(self):
        conn = self.get_connection_combobox_opt()
        auth = self.get_auth_combobox_opt()
        if not conn or not auth:
            return None
        
        return dict(profile_name=self.view['profile_name_entry'].get_text(),
                    username=self.view['username_entry'].get_text(),
                    password=self.view['password_entry'].get_text(),
                    connection=CONN_OPTS_DICT[conn],
                    apn=self.view['apn_entry'].get_text(),
                    dialer_profile=AUTH_OPTS_DICT[auth],
                    staticdns=self.view['dns_checkbutton'].get_active(),
                    dns1=self.dns1_entry.get_text(),
                    dns2=self.dns2_entry.get_text())
    
    def on_cancel_button_clicked(self, widget):
        self.hide_ourselves()
        if hasattr(self, 'startup') and self.startup:
            shutdown_core()
    
    def on_dns_checkbutton_toggled(self, widget):
        self.view['vbox3'].set_sensitive(widget.get_active())
    
    def on_connection_combobox_changed(self, combobox):
        if not self.view:
            # we have to load first the view and then register it with it with
            # the controller, however if we register the view before building
            # it, the combobox models wont be ready. If we do it this way,
            # the models will be ready, but when we propose the combobox opts
            # while creating a new profile, self.view will be == None, it's
            # all good thou
            return
        
        if not self.the_name_is_custom():
            self.propose_profile_name()
    
    def the_name_is_custom(self):
        name = self.view['profile_name_entry'].get_text()
        return name != self.suggested_name
    
    def hide_ourselves(self):
        self.model.unregister_observer(self)
        self.view.get_top_widget().destroy()
        self.view = None
        self.model = None

class NewProfileController(BaseProfileController):
    """
    Controller for the initial configuration window
    """
    def __init__(self, model, startup=False, hotplug=False, aux_ctrl=None):
        super(NewProfileController, self).__init__(model)
        self.startup = startup
        self.hotplug = hotplug
        self.aux_ctrl = aux_ctrl
    
    def on_ok_button_clicked(self, widget):
        settings = self.get_profile_settings()
        
        if not settings:
            # preferred connection or auth not specified
            return
        
        if settings['staticdns']:
            if not self.dns1_entry.isvalid() or not self.dns2_entry.isvalid():
                # DNS must be valid
                return
        
        if not settings['profile_name'] or settings['profile_name'] == '':
            self.view['profile_name_entry'].grab_focus()
            return
        
        if self.is_profile_name_insane():
            message = _('Invalid characters in profile name')
            details = _("""
The following characters are not allowed in a profile name:
%s""") % ' '.join(INVALID_CHARS)
            dialogs.open_warning_dialog(message, details)
            return
        
        from vmc.common.config import config
        prof_manager = get_profile_manager(self.model.get_device())
        prof = prof_manager.create_profile(settings['profile_name'], settings)
        config.set_current_profile(prof)
        
        if self.startup:
            self.aux_ctrl.start()
        
        elif self.hotplug:
            self.aux_ctrl.model.wrapper.start_behaviour(self.aux_ctrl)
        
        # now hide
        self.hide_ourselves()


class EditProfileController(BaseProfileController):
    """
    Controller to edit mobile profiles
    """
    def __init__(self, model, profile):
        super(EditProfileController, self).__init__(model, quiet=True)
        self.profile = profile
    
    def register_view(self, view):
        super(EditProfileController, self).register_view(view)
        self.load_mobile_profile(self.profile)
    
    def on_ok_button_clicked(self, widget):
        settings = self.get_profile_settings()
        
        if not settings:
            # preferred connection or auth not specified
            return
        
        if settings['staticdns']:
            if not self.dns1_entry.isvalid() or not self.dns2_entry.isvalid():
                # DNS must be valid
                return
        
        if not settings['profile_name'] or settings['profile_name'] == '':
            self.view['profile_name_entry'].grab_focus()
            return
        
        if self.is_profile_name_insane():
            message = _('Invalid characters in profile name')
            details = _("""
The following characters are not allowed in a profile name:
%s""") % ' '.join(INVALID_CHARS)
            dialogs.open_warning_dialog(message, details)
            return
        
        prof_manager = get_profile_manager(self.model.get_device())
        prof_manager.edit_profile(self.profile, settings)
        
        # now hide
        self.hide_ourselves()


class DeviceSelectionController(Controller, DbusComponent):
    """
    Controller for the device selection window
    """
    def __init__(self, model, device_list, device_callback, splash=None):
        Controller.__init__(self, model)
        DbusComponent.__init__(self)
        self.device_list = device_list
        self.device_callback = device_callback
        self.splash = splash
        self.register_handlers()
        self.udi_device = {}
        for device in device_list:
            if hasattr(device, 'udi'):
                self.udi_device[device.udi] = device

    def register_handlers(self):
        connect = self.manager.connect_to_signal
        self.handlers = []
        self.handlers.append(connect('DeviceAdded', self.device_added))
        self.handlers.append(connect('DeviceRemoved', self.device_removed))

    def unregister_handlers(self):
        for handler in self.handlers:
            handler.remove()

    def device_added(self, udi):
        def process_device_added():
            properties = self.get_devices_properties()
            cached_id = None

            for device in DeviceProfileCache.get_cached_devices():
                if device in properties:
                    cached_id = device.cached_id

            for plugin in PluginManager.get_plugins(IDBusDevicePlugin):
                if plugin in properties:
                    try:
                        plugin.setup(properties)
                        if cached_id:
                            plugin.cached_id = cached_id
                        if (plugin.udi in self.udi_device):
                            continue
                        self.udi_device[str(plugin.udi)] = plugin
                        d = DeviceResolver.identify_device(plugin)
                        def select_and_configure(name):
                            try:
                                plugin = \
                                  PluginManager.get_plugin_by_remote_name(name)
                                plugin.setup(properties)
                                self.udi_device[str(plugin.udi)] = plugin
                                self.device_list.append(plugin)
                                self.view.device_added(plugin)
                            except Exception, e:
                                print e
                        d.addCallback(select_and_configure)
                    except Exception, e:
                        pass

        reactor.callLater(4, process_device_added)

    def device_removed(self, udi):
        if self.udi_device.has_key(udi):
            device = self.udi_device[udi]
            del(self.udi_device[udi])
            self.device_list.remove(device)
            self.view.device_removed(device)

    def on_device_selection_window_delete_event(self, widget, userdata):
        self.hide_ourselves()
        shutdown_core()

    def on_ok_button_clicked(self, widget):
        def get_selected_device_eb(failure):
            failure.trap(SerialException)
            message = _('Device setup not completed')
            details = _("""
An unknown error occur when setting up the device:
%s""") % failure.getErrorMessage()
            dialogs.open_warning_dialog(message, details)

        def get_selected_device_cb(device):
            self.hide_ourselves()
            self.device_callback(device)
        
        d = self.view.get_selected_device()
        d.addCallback(get_selected_device_cb)
        d.addErrback(get_selected_device_eb)
    
    def on_cancel_button_clicked(self, widget):
        self.hide_ourselves()
        shutdown_core()

    def on_custom_device_radio_toggled(self, widget):
        if widget.get_active():
            self.view.enable_custom_device_controls()
            self.view.disable_known_device_controls()

    def on_known_device_radio_toggled(self, widget):
        if widget.get_active():
            self.view.enable_known_device_controls()
            self.view.disable_custom_device_controls()

    def hide_ourselves(self):
        self.view.get_top_widget().destroy()
        self.unregister_handlers()
        self.manager = None
        self.view = None


IDLE, CONNECTING, CONNECTED = range(3)

class BluetoothConfController2(Controller):
    """
    Controller for the initial configuration window
    """
    def __init__(self, model, sview=None):
        Controller.__init__(self, model)
        self.sview = sview
        self.disc_queue = None
        self.callback = None
        self.state = IDLE
    
    def register_view(self, view):
        Controller.register_view(self, view)
        treeview = self.view['device_treeview']
        treeview.connect('row-activated', self._row_activated_handler)
    
    def _row_activated_handler(self, treeview, path, col):
        model, selected = treeview.get_selection().get_selected_rows()
        if not selected:
            return
        
        if self.state != IDLE:
            return
        
        _iter = model.get_iter(selected[0])
        name = model.get_value(_iter, 0)
        address = model.get_value(_iter, 1)
        
        def create_bonding_eb(failure):
            msg = _('Authentication Error from %s !') % address
            self.view['statusbar1'].push(1, msg)
            dialogs.open_warning_dialog(msg, failure.getErrorMessage())
        
        self.view['statusbar1'].push(1, _('Connecting to %s ...') % address)
        d = self.model.create_bonding(address)
        d.addCallback(lambda address: self._obtain_addresses(address))
        d.addErrback(create_bonding_eb)
    
    def _process_port_info(self, port, name):
        if not port:
            return
        
        try:
            info = port.GetInfo()
        except:
            return
        
        print "INFO", info
        
        if name == 'data':
            combo = self.view['data_comboboxentry']
        else:
            combo = self.view['control_comboboxentry']
        
        model = combo.get_model()
        _iter = model.get_iter_first()
        model.insert_before(_iter, [str(info['device'])])
        combo.set_active(0)
    
    def _obtain_addresses(self, address):
        d = threads.deferToThread(self.model.get_sports, address)
        self.state = CONNECTING
        
        def get_sports_cb((dport, cport)):
            if not dport and not cport:
                pass
            
            self._process_port_info(dport, 'data')
            self._process_port_info(cport, 'control')
            
            self.view['forward_button'].set_sensitive(True)
        
        d.addCallback(get_sports_cb)
    
    def on_back_button_clicked(self, widget):
        print "BACK"
    
    def on_start_button_clicked(self, widget):
        self.disc_queue = self.model.get_bluetooth_discv_queue()
        self.disc_queue.get().addCallback(self._fill_treeview_with_device)
        
        self.view['statusbar1'].push(1, _('Discovery started...'))
    
    def _fill_treeview_with_device(self, device):
        if not device:
            self.view['statusbar1'].push(1, _('Discovery finished!'))
            return
        
        model = self.view['device_treeview'].get_model()
        model.add_device(device)
        self.disc_queue.get().addCallback(self._fill_treeview_with_device)
    
    def on_help_button_clicked(self, widget):
        print "HELP"
    
    def _get_selected_opt_from_combobox(self, comboname):
        comboname += '_comboboxentry'
        model = self.view[comboname].get_model()
        index = self.view[comboname].get_active()
        if index < 0:
            return None
        else:
            return model[index][0]
    
    def on_forward_button_clicked(self, widget):
        dport = self._get_selected_opt_from_combobox('data')
        cport = self._get_selected_opt_from_combobox('control')
        speed = int(self._get_selected_opt_from_combobox('speed'))
        
        if cport == _("No control port"):
            cport = None
        
        software = self.view['software_checkbutton'].get_active()
        hardware = self.view['hardware_checkbutton'].get_active()
        
        title = _('Connecting to device...')
        apb = dialogs.ActivityProgressBar(title, self, True)
        
        def get_remote_plugin_eb(failure):
            failure.trap(SerialException)
            apb.close()
            
            port = cport and cport or dport
            message = _('Exception received connecting to %s') % port
            details = _("""
The following error was received while trying to establish a connection:
%s""") % failure.getErrorMessage()
            dialogs.open_warning_dialog(message, details)
        
        from vmc.common.hardware import HardwareRegistry
        hw = HardwareRegistry()
        
        d = hw.get_plugin_for_remote_dev(speed, dport, cport)
        d.addCallback(self._im_done, apb)
        d.addErrback(get_remote_plugin_eb)
        
    def on_cancel_button_clicked(self, widget):
        # the user has pressed cancel, we're gonna close the program
        title = _('Shutting down')
        apb = dialogs.ActivityProgressBar(title, self)
        
        def default_eb():
            pass
        
        apb.set_default_cb(2, lambda: shutdown_core(delay=.3))
        apb.set_cancel_cb(default_eb)
        apb.init()
    
    def _im_done(self, device, widget=None):
        if widget:
            widget.close()
        
        self.callback(device)
        self.model.unregister_observer(self)
        self.view.hide()

class BluetoothConfController(Controller):
    """
    Controller for the initial configuration window
    """
    def __init__(self, model, sview=None):
        Controller.__init__(self, model)
        self.sview = sview
    
    def on_back_button_clicked(self, widget):
        print "BACK"
    
    def on_ok_button_clicked(self, widget):
        print "OK"
    
    def _row_activated_handler(self, treeview, path, col):
        model, selected = treeview.get_selection().get_selected_rows()
        if not selected:
            return
        
        _iter = model.get_iter(selected[0])
        name = model.get_value(_iter, 0)
        address = model.get_value(_iter, 1)
        
        d = self.model.create_bonding(address)
        def create_bonding_cb(ignored):
            self.model.connect_to_device(address)
            
        d.addCallback(create_bonding_cb)
        
    
    def _setup_treeview(self, devices):
        # setup treeview
        treeview = self.view['device_treeview']
        treeview.connect('row-activated', self._row_activated_handler)
        model = treeview.get_model()
        for device in devices:
            model.add_device(device)
        
        if devices:
            self.view['expander1'].set_sensitive(True)
            self.view['expander1'].set_expanded(True)
    
    def on_discovery_button_clicked(self, widget):
        d = self.model.get_bluetooth_devices()
        def discovery_cb(devices):
            print "DEVICES", devices
            self._setup_treeview(devices)
        
        d.addCallback(discovery_cb)
        
    def on_cancel_button_clicked(self, widget):
        print "CANCEL"
