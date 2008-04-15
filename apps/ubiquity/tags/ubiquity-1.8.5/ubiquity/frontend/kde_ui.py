# -*- coding: utf-8 -*-
#
# Copyright (C) 2006, 2007 Canonical Ltd.
#
# Author(s):
#   Jonathan Riddell <jriddell@ubuntu.com>
#   Mario Limonciello <superm1@ubuntu.com>
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

import sys
import os
import datetime
import subprocess
import math
import traceback
import syslog
import atexit
import signal
import gettext

#from qt import *
from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import uic
#from kdeui import *
#from kdecore import *
#from kio import KRun
#import kdedesigner

import debconf

from ubiquity import filteredcommand, i18n, validation
from ubiquity.misc import *
from ubiquity.components import console_setup, language, timezone, usersetup, \
                                partman, partman_commit, summary, install
import ubiquity.tz
import ubiquity.progressposition
from ubiquity.frontend.base import BaseFrontend

from PartitionsBarKde import *

# Define global path
PATH = '/usr/share/ubiquity'

# Define locale path
LOCALEDIR = "/usr/share/locale"

UIDIR = os.path.join(PATH, 'qt')

BREADCRUMB_STEPS = {
    "stepLanguage": 1,
    "stepLocation": 2,
    "stepKeyboardConf": 3,
    "stepPartAuto": 4,
    "stepPartAdvanced": 4,
    "stepUserInfo": 5,
    "stepReady": 6
}
BREADCRUMB_MAX_STEP = 6

WIDGET_STACK_STEPS = {
    "stepWelcome": 0,
    "stepLanguage": 1,
    "stepLocation": 2,
    "stepKeyboardConf": 3,
    "stepPartAuto": 4,
    "stepPartAdvanced": 5,
    "stepUserInfo": 6,
    "stepReady": 7
}

class UbiquityUI(QWidget):

    def __init__(self, parent):
        QWidget.__init__(self, parent)
        uic.loadUi("%s/liveinstaller.ui" % UIDIR, self)

    def setWizard(self, wizardRef):
        self.wizard = wizardRef

    def closeEvent(self, event):
        if self.wizard.on_cancel_clicked() == False:
            event.ignore()

class linkLabel(QLabel):

    def __init__(self, wizard, parent):
        QLabel.__init__(self, parent)
        self.wizard = wizard

    def mouseReleaseEvent(self, event):
        self.wizard.openReleaseNotes()

    def setText(self, text):
        QLabel.setText(self, text)
        self.resize(self.sizeHint())

class Wizard(BaseFrontend):

    def __init__(self, distro):
        BaseFrontend.__init__(self, distro)

        self.previous_excepthook = sys.excepthook
        sys.excepthook = self.excepthook

        #about=KAboutData("kubuntu-ubiquity","Installer","0.1","Live CD Installer for Kubuntu",KAboutData.License_GPL,"(c) 2006 Canonical Ltd", "http://wiki.kubuntu.org/KubuntuUbiquity", "jriddell@ubuntu.com")
        #about.addAuthor("Jonathan Riddell", None,"jriddell@ubuntu.com")
        #KCmdLineArgs.init(["./installer"],about)

        #self.app = KApplication()

        self.app = QApplication(['ubiquity', '-style=plastique'])

        # We want to hide the minimise button if running in the ubiquity-only mode (no desktop)
        # To achieve this we need to set window flags to Dialog but we also need a parent widget which is showing
        # else Qt tried to be clever and puts the minimise button back
        self.parentWidget = QWidget()
        if 'UBIQUITY_ONLY' in os.environ:
            self.parentWidget.show()
        self.userinterface = UbiquityUI(self.parentWidget)
        self.userinterface.setWizard(self)
        self.userinterface.setWindowFlags(Qt.Dialog)
        #self.app.setMainWidget(self.userinterface)

        self.advanceddialog = QDialog(self.userinterface)
        uic.loadUi("%s/advanceddialog.ui" % UIDIR, self.advanceddialog)

        # declare attributes
        self.release_notes_url_template = None
        self.language_questions = ('live_installer',
                                   'welcome_heading_label', 'welcome_text_label',
                                   'oem_id_label',
                                   'release_notes_label', 'release_notes_url',
                                   'step_label',
                                   'cancel', 'back', 'next')
        self.current_page = None
        self.first_seen_page = None
        self.allowed_change_step = True
        self.allowed_go_forward = True
        self.stay_on_page = False
        self.mainLoopRunning = False
        self.progressDialogue = None
        self.progress_position = ubiquity.progressposition.ProgressPosition()
        self.progress_cancelled = False
        self.resize_min_size = None
        self.resize_max_size = None
        self.new_size_value = None
        self.new_size_scale = None
        self.username_edited = False
        self.hostname_edited = False
        self.previous_partitioning_page = WIDGET_STACK_STEPS["stepPartAuto"]
        self.grub_en = True
        self.installing = False
        self.installing_no_return = False
        self.returncode = 0

        self.laptop = execute("laptop-detect")
        self.partition_tree_model = None
        self.app.connect(self.userinterface.partition_list_treeview, SIGNAL("customContextMenuRequested(const QPoint&)"), self.partman_popup)
        self.app.connect(self.userinterface.partition_list_treeview, SIGNAL("activated(const QModelIndex&)"), self.on_partition_list_treeview_activated)

        # set default language
        dbfilter = language.Language(self, self.debconf_communicator())
        dbfilter.cleanup()
        dbfilter.db.shutdown()

        self.debconf_callbacks = {}    # array to keep callback functions needed by debconf file descriptors

        self.map_vbox = QVBoxLayout(self.userinterface.map_frame)
        self.map_vbox.setMargin(0)

        self.customize_installer()

        release_notes_layout = QHBoxLayout(self.userinterface.release_notes_frame)
        self.release_notes_url = linkLabel(self, self.userinterface.release_notes_frame)
        self.release_notes_url.setObjectName("release_notes_url")
        self.release_notes_url.show()

        self.translate_widgets()

        self.autopartition_vbox = QVBoxLayout(self.userinterface.autopartition_frame)
        self.autopartition_buttongroup = QButtonGroup(self.userinterface.autopartition_frame)
        self.autopartition_buttongroup_texts = {}
        self.autopartition_handlers = {}
        self.autopartition_extras = {}
        self.autopartition_extra_buttongroup = {}
        self.autopartition_extra_buttongroup_texts = {}

        self.partition_bar_vbox = QVBoxLayout(self.userinterface.partition_bar_frame)
        self.partition_bar_vbox.setSpacing(0)
        self.partition_bar_vbox.setMargin(0)

        if os.path.exists("/usr/lib/kde4/share/icons/oxygen/32x32/status/dialog-warning.png"):
            warningIcon = QPixmap("/usr/lib/kde4/share/icons/oxygen/32x32/status/dialog-warning.png")
        else:
            warningIcon = QPixmap("/usr/share/icons/crystalsvg/32x32/actions/messagebox_warning.png")
        self.userinterface.fullname_error_image.setPixmap(warningIcon)
        self.userinterface.username_error_image.setPixmap(warningIcon)
        self.userinterface.password_error_image.setPixmap(warningIcon)
        self.userinterface.hostname_error_image.setPixmap(warningIcon)

        if os.path.exists("/usr/lib/kde4/share/icons/oxygen/16x16/actions/go-next.png"):
            self.forwardIcon = QIcon("/usr/lib/kde4/share/icons/oxygen/16x16/actions/go-next.png")
        else:
            self.forwardIcon = QIcon("/usr/share/icons/crystalsvg/16x16/actions/forward.png")
        self.userinterface.next.setIcon(self.forwardIcon)

        #Used for the last step
        if os.path.exists("/usr/lib/kde4/share/icons/oxygen/16x16/actions/dialog-ok-apply.png"):
            self.applyIcon = QIcon("/usr/lib/kde4/share/icons/oxygen/16x16/actions/dialog-ok-apply.png")
        else:
            self.applyIcon = QIcon("/usr/share/icons/crystalsvg/16x16/actions/ok.png")

        if os.path.exists("/usr/lib/kde4/share/icons/oxygen/16x16/actions/go-previous.png"):
            backIcon = QIcon("/usr/lib/kde4/share/icons/oxygen/16x16/actions/go-previous.png")
        else:
            backIcon = QIcon("/usr/share/icons/crystalsvg/16x16/actions/back.png")
        self.userinterface.back.setIcon(backIcon)

        if os.path.exists("/usr/lib/kde4/share/icons/oxygen/16x16/actions/dialog-cancel.png"):
            cancelIcon = QIcon("/usr/lib/kde4/share/icons/oxygen/16x16/actions/dialog-cancel.png")
        else:
            cancelIcon = QIcon("/usr/share/icons/crystalsvg/22x22/actions/button_cancel.png")
        self.userinterface.cancel.setIcon(cancelIcon)

    def excepthook(self, exctype, excvalue, exctb):
        """Crash handler."""

        if (issubclass(exctype, KeyboardInterrupt) or
            issubclass(exctype, SystemExit)):
            return

        tbtext = ''.join(traceback.format_exception(exctype, excvalue, exctb))
        syslog.syslog(syslog.LOG_ERR,
                      "Exception in KDE frontend (invoking crash handler):")
        for line in tbtext.split('\n'):
            syslog.syslog(syslog.LOG_ERR, line)
        print >>sys.stderr, ("Exception in KDE frontend"
                             " (invoking crash handler):")
        print >>sys.stderr, tbtext

        self.post_mortem(exctype, excvalue, exctb)

        if os.path.exists('/usr/share/apport/apport-qt'):
            self.previous_excepthook(exctype, excvalue, exctb)
        else:
            dialog = QDialog(self.userinterface)
            uic.loadUi("%s/crashdialog.ui" % UIDIR, dialog)
            dialog.beastie_url.setOpenExternalLinks(True)
            dialog.crash_detail.setText(tbtext)
            dialog.exec_()
            sys.exit(1)

    # Disable the KDE media notifier to avoid problems during partitioning.
    def disable_volume_manager(self):
        execute('dcop', 'kded', 'kded', 'unloadModule', 'medianotifier')
        atexit.register(self.enable_volume_manager)

    def enable_volume_manager(self):
        execute('dcop', 'kded', 'kded', 'loadModule', 'medianotifier')

    def openReleaseNotes(self):
        self.openURL(self.release_notes_url_template)

    def openURL(self, url):
        #need to run this else kdesu can't run Konqueror
        execute('su', 'ubuntu', 'xhost', '+localhost')
        execute('su', 'ubuntu', 'xdg-open', url)

    def run(self):
        """run the interface."""

        if os.getuid() != 0:
            title = ('This installer must be run with administrative '
                     'privileges, and cannot continue without them.')
            result = QMessageBox.critical(self.userinterface, "Must be root",
                                          title)
            sys.exit(1)

        self.disable_volume_manager()

        # show interface
        # TODO cjwatson 2005-12-20: Disabled for now because this segfaults in
        # current dapper (https://bugzilla.ubuntu.com/show_bug.cgi?id=20338).
        #self.show_browser()
        got_intro = self.show_intro()
        self.allow_change_step(True)

        # Declare SignalHandler
        self.app.connect(self.userinterface.next, SIGNAL("clicked()"), self.on_next_clicked)
        self.app.connect(self.userinterface.back, SIGNAL("clicked()"), self.on_back_clicked)
        self.app.connect(self.userinterface.cancel, SIGNAL("clicked()"), self.on_cancel_clicked)
        self.app.connect(self.userinterface.keyboardlayoutview, SIGNAL("itemSelectionChanged()"), self.on_keyboard_layout_selected)
        self.app.connect(self.userinterface.keyboardvariantview, SIGNAL("itemSelectionChanged()"), self.on_keyboard_variant_selected)

        self.app.connect(self.userinterface.fullname, SIGNAL("textChanged(const QString &)"), self.on_fullname_changed)
        self.app.connect(self.userinterface.username, SIGNAL("textChanged(const QString &)"), self.on_username_changed)
        self.app.connect(self.userinterface.username, SIGNAL("textChanged(const QString &)"), self.on_username_insert_text)
        self.app.connect(self.userinterface.password, SIGNAL("textChanged(const QString &)"), self.on_password_changed)
        self.app.connect(self.userinterface.verified_password, SIGNAL("textChanged(const QString &)"), self.on_verified_password_changed)
        self.app.connect(self.userinterface.hostname, SIGNAL("textChanged(const QString &)"), self.on_hostname_changed)
        self.app.connect(self.userinterface.hostname, SIGNAL("textChanged(const QString &)"), self.on_hostname_insert_text)

        self.app.connect(self.userinterface.fullname, SIGNAL("selectionChanged()"), self.on_fullname_changed)
        self.app.connect(self.userinterface.username, SIGNAL("selectionChanged()"), self.on_username_changed)
        self.app.connect(self.userinterface.password, SIGNAL("selectionChanged()"), self.on_password_changed)
        self.app.connect(self.userinterface.verified_password, SIGNAL("selectionChanged()"), self.on_verified_password_changed)
        self.app.connect(self.userinterface.hostname, SIGNAL("selectionChanged()"), self.on_hostname_changed)

        self.app.connect(self.userinterface.language_treeview, SIGNAL("itemSelectionChanged()"), self.on_language_treeview_selection_changed)

        self.app.connect(self.userinterface.timezone_city_combo, SIGNAL("activated(int)"), self.tzmap.city_combo_changed)

        self.app.connect(self.userinterface.advanced_button, SIGNAL("clicked()"), self.on_advanced_button_clicked)

        self.app.connect(self.userinterface.partition_button_new_label, SIGNAL("clicked(bool)"), self.on_partition_list_new_label_activate)
        self.app.connect(self.userinterface.partition_button_new, SIGNAL("clicked(bool)"), self.on_partition_list_new_activate)
        self.app.connect(self.userinterface.partition_button_edit, SIGNAL("clicked(bool)"),self.on_partition_list_edit_activate)
        self.app.connect(self.userinterface.partition_button_delete, SIGNAL("clicked(bool)"),self.on_partition_list_delete_activate)
        self.app.connect(self.userinterface.partition_button_undo, SIGNAL("clicked(bool)"),self.on_partition_list_undo_activate)

        self.pages = [language.Language, timezone.Timezone,
            console_setup.ConsoleSetup, partman.Partman,
            usersetup.UserSetup, summary.Summary]

        self.pagesindex = 0
        pageslen = len(self.pages)

        if 'UBIQUITY_AUTOMATIC' in os.environ:
            got_intro = False
            self.debconf_progress_start(0, pageslen,
                self.get_string('ubiquity/install/checking'))
            self.refresh()

        # Start the interface
        if got_intro:
            global BREADCRUMB_STEPS, BREADCRUMB_MAX_STEP
            for step in BREADCRUMB_STEPS:
                BREADCRUMB_STEPS[step] += 1
            BREADCRUMB_STEPS["stepWelcome"] = 1
            BREADCRUMB_MAX_STEP += 1
            first_step = "stepWelcome"
        else:
            first_step = "stepLanguage"
        self.set_current_page(WIDGET_STACK_STEPS[first_step])
        
        if got_intro:
            self.app.exec_()
        
        while(self.pagesindex < pageslen):
            if self.current_page == None:
                break

            self.backup = False
            old_dbfilter = self.dbfilter
            self.dbfilter = self.pages[self.pagesindex](self)

            # Non-debconf steps are no longer possible as the interface is now
            # driven by whether there is a question to ask.
            if self.dbfilter is not None and self.dbfilter != old_dbfilter:
                self.allow_change_step(False)
                self.dbfilter.start(auto_process=True)
            self.app.exec_()

            if self.backup or self.dbfilter_handle_status():
                if self.installing:
                    self.progress_loop()
                elif self.current_page is not None and not self.backup:
                    self.process_step()
                    if not self.stay_on_page:
                        self.pagesindex = self.pagesindex + 1
                    if 'UBIQUITY_AUTOMATIC' in os.environ:
                        # if no debconf_progress, create another one, set start to pageindex
                        self.debconf_progress_step(1)
                        self.refresh()
                if self.backup:
                    if self.pagesindex > 0:
                        step = self.step_name(self.get_current_page())
                        if not step == "stepPartAdvanced": #Advanced will already have pagesindex pointing at first Paritioning page
                            self.pagesindex = self.pagesindex - 1

            self.app.processEvents()

            # needed to be here for --automatic as there might not be any
            # current page in the event all of the questions have been
            # preseeded.
            if self.pagesindex == pageslen:
                # Ready to install
                self.current_page = None
                self.installing = True
                self.progress_loop()
        return self.returncode

    def customize_installer(self):
        """Initial UI setup."""

        self.userinterface.setWindowIcon(QIcon("/usr/share/icons/hicolor/64x64/apps/ubiquity.png"))
        self.userinterface.back.hide()

        """
        PIXMAPSDIR = os.path.join(PATH, 'pixmaps', self.distro)

        # set pixmaps
        if ( gtk.gdk.get_default_root_window().get_screen().get_width() > 1024 ):
            logo = os.path.join(PIXMAPSDIR, "logo_1280.jpg")
            photo = os.path.join(PIXMAPSDIR, "photo_1280.jpg")
        else:
            logo = os.path.join(PIXMAPSDIR, "logo_1024.jpg")
            photo = os.path.join(PIXMAPSDIR, "photo_1024.jpg")
        if not os.path.exists(logo):
            logo = None
        if not os.path.exists(photo):
            photo = None

        self.logo_image.set_from_file(logo)
        self.photo.set_from_file(photo)
        """

        if self.oem_config:
            self.userinterface.setWindowTitle(
                self.get_string('oem_config_title'))
            try:
                self.userinterface.oem_id_entry.setText(
                    self.debconf_operation('get', 'oem-config/id'))
            except debconf.DebconfError:
                pass
            self.userinterface.fullname.setText(
                'OEM Configuration (temporary user)')
            self.userinterface.fullname.setReadOnly(True)
            self.userinterface.fullname.setEnabled(False)
            self.userinterface.username.setText('oem')
            self.userinterface.username.setReadOnly(True)
            self.userinterface.username.setEnabled(False)
            self.username_edited = True
            # The UserSetup component takes care of preseeding passwd/user-uid.
            execute_root('apt-install', 'oem-config-kde')
        else:
            self.userinterface.oem_id_label.hide()
            self.userinterface.oem_id_entry.hide()
        
        if not 'UBIQUITY_AUTOMATIC' in os.environ:
            self.userinterface.show()
            self.parentWidget.hide()

        try:
            release_notes = open('/cdrom/.disk/release_notes_url')
            self.release_notes_url_template = release_notes.read().rstrip('\n')
            release_notes.close()
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            self.userinterface.release_notes_label.hide()
            self.userinterface.release_notes_frame.hide()

        self.tzmap = TimezoneMap(self)
        self.tzmap.tzmap.show()

        self.userinterface.password_debug_warning_label.setVisible(
            'UBIQUITY_DEBUG' in os.environ)

    def translate_widgets(self, parentWidget=None):
        if self.locale is None:
            languages = []
        else:
            languages = [self.locale]
        core_names = ['ubiquity/text/%s' % q for q in self.language_questions]
        core_names.append('ubiquity/text/oem_config_title')
        for stock_item in ('cancel', 'close', 'go-back', 'go-forward',
                           'ok', 'quit'):
            core_names.append('ubiquity/imported/%s' % stock_item)
        i18n.get_translations(languages=languages, core_names=core_names)

        self.translate_widget_children(parentWidget)

        self.userinterface.partition_button_undo.setText(
            self.get_string('partman/text/undo_everything').replace('_', '&', 1))
        if self.release_notes_url_template is not None:
            url = self.release_notes_url_template.replace('${LANG}', self.locale.split('.')[0])
            text = self.get_string('release_notes_url')
            self.release_notes_url.setText('<a href="%s">%s</a>' % (url, text))

    def translate_widget_children(self, parentWidget=None):
        if parentWidget == None:
            parentWidget = self.userinterface

        self.translate_widget(parentWidget, self.locale)
        if parentWidget.children() != None:
            for widget in parentWidget.children():
                self.translate_widget_children(widget)

    def translate_widget(self, widget, lang):
        #FIXME needs translations for Next, Back and Cancel
        if not isinstance(widget, QWidget):
            return

        name = str(widget.objectName())

        text = self.get_string(name, lang)

        if str(name) == "UbiquityUIBase":
            text = self.get_string("live_installer", lang)

        if text is None:
            return

        if isinstance(widget, QLabel):
            if name == 'step_label':
                global BREADCRUMB_STEPS, BREADCRUMB_MAX_STEP
                curstep = '?'
                if self.current_page is not None:
                    current_name = self.step_name(self.current_page)
                    if current_name in BREADCRUMB_STEPS:
                        curstep = str(BREADCRUMB_STEPS[current_name])
                text = text.replace('${INDEX}', curstep)
                text = text.replace('${TOTAL}', str(BREADCRUMB_MAX_STEP))

            if 'heading_label' in name:
                widget.setText("<h2>" + text + "</h2>")
            elif 'extra_label' in name:
                widget.setText("<em>" + text + "</em>")
            elif ('group_label' in name or 'warning_label' in name or
                  name in ('drives_label', 'partition_method_label')):
                widget.setText("<strong>" + text + "</strong>")
            elif name == 'release_notes_url':
                if self.release_notes_url_template is not None:
                    url = self.release_notes_url_template.replace(
                        '${LANG}', lang.split('.')[0])
                    widget.setText('<a href="%s">%s</a>' % (url, text))
            else:
                widget.setText(text)

        elif isinstance(widget, QPushButton) or isinstance(widget, QCheckBox):
            widget.setText(text.replace('_', '&', 1))

        elif isinstance(widget, QWidget) and str(name) == "UbiquityUIBase":
            if self.oem_config:
                text = self.get_string('oem_config_title', lang)
            widget.setWindowTitle(text)

        else:
            print "WARNING: unknown widget: " + name

    def allow_change_step(self, allowed):
        if allowed:
            cursor = QCursor(Qt.ArrowCursor)
        else:
            cursor = QCursor(Qt.WaitCursor)
        self.userinterface.setCursor(cursor)
        self.userinterface.back.setEnabled(allowed)
        self.userinterface.next.setEnabled(allowed and self.allowed_go_forward)
        self.allowed_change_step = allowed

    def allow_go_forward(self, allowed):
        self.userinterface.next.setEnabled(allowed and self.allowed_change_step)
        self.allowed_go_forward = allowed

    def dbfilter_handle_status(self):
        """If a dbfilter crashed, ask the user if they want to continue anyway.

        Returns True to continue, or False to try again."""

        if not self.dbfilter_status or self.current_page is None:
            return True

        syslog.syslog('dbfilter_handle_status: %s' % str(self.dbfilter_status))

        # TODO cjwatson 2007-04-04: i18n
        text = ('%s failed with exit code %s. Further information may be '
                'found in /var/log/syslog. Do you want to try running this '
                'step again before continuing? If you do not, your '
                'installation may fail entirely or may be broken.' %
                (self.dbfilter_status[0], self.dbfilter_status[1]))
        #FIXME QMessageBox seems to have lost the ability to set custom labels
        # so for now we have to get by with these not-entirely meaningful stock labels
        answer = QMessageBox.warning(self.userinterface,
                                     '%s crashed' % self.dbfilter_status[0],
                                     text, QMessageBox.Retry,
                                     QMessageBox.Ignore, QMessageBox.Close)
        self.dbfilter_status = None
        syslog.syslog('dbfilter_handle_status: answer %d' % answer)
        if answer == QMessageBox.Ignore:
            return True
        elif answer == QMessageBox.Close:
            self.quit()
        else:
            step = self.step_name(self.get_current_page())
            if str(step).startswith("stepPart"):
                self.set_current_page(WIDGET_STACK_STEPS["stepPartAuto"])
            return False

    def show_intro(self):
        """Show some introductory text, if available."""

        intro = os.path.join(PATH, 'intro.txt')

        if os.path.isfile(intro):
            intro_file = open(intro)
            text = ""
            for line in intro_file:
                text = text + line + "<br>"
            self.userinterface.introLabel.setText(text)
            intro_file.close()
            return True
        else:
            return False

    def step_name(self, step_index):
        if step_index < 0:
            step_index = 0
        return str(self.userinterface.widgetStack.widget(step_index).objectName())

    def set_page(self, n):
        self.run_automation_error_cmd()
        self.userinterface.show()
        if n == 'Language':
            self.set_current_page(WIDGET_STACK_STEPS["stepLanguage"])
        elif n == 'ConsoleSetup':
            self.set_current_page(WIDGET_STACK_STEPS["stepKeyboardConf"])
        elif n == 'Timezone':
            self.set_current_page(WIDGET_STACK_STEPS["stepLocation"])
        elif n == 'Partman':
            # Rather than try to guess which partman page we should be on,
            # we leave that decision to set_autopartitioning_choices and
            # update_partman.
            return
        elif n == 'UserSetup':
            self.set_current_page(WIDGET_STACK_STEPS["stepUserInfo"])
        elif n == 'Summary':
            self.set_current_page(WIDGET_STACK_STEPS["stepReady"])
            self.userinterface.next.setText(self.get_string('install_button').replace('_', '&', 1))
            self.userinterface.next.setIcon(self.applyIcon)
        else:
            print >>sys.stderr, 'No page found for %s' % n
            return

        if not self.first_seen_page:
            self.first_seen_page = n
        if self.first_seen_page == self.pages[self.pagesindex].__name__:
            self.userinterface.back.hide()
        else:
            self.userinterface.back.show()
    
    def set_current_page(self, current):
        widget = self.userinterface.widgetStack.widget(current)
        if self.userinterface.widgetStack.currentWidget() == widget:
            # self.userinterface.widgetStack.raiseWidget() will do nothing.
            # Update state ourselves.
            self.on_steps_switch_page(current)
        else:
            self.userinterface.widgetStack.setCurrentWidget(widget)
            self.on_steps_switch_page(current)

    def progress_loop(self):
        """prepare, copy and config the system in the core install process."""

        syslog.syslog('progress_loop()')

        self.current_page = None

        self.debconf_progress_start(
            0, 100, self.get_string('ubiquity/install/title'))
        self.debconf_progress_region(0, 15)

        dbfilter = partman_commit.PartmanCommit(self)
        if dbfilter.run_command(auto_process=True) != 0:
            while self.progress_position.depth() != 0:
                self.debconf_progress_stop()
            self.progressDialogue.hide()
            self.return_to_partitioning()
            return

        # No return to partitioning from now on
        self.installing_no_return = True

        self.debconf_progress_region(15, 100)

        dbfilter = install.Install(self)
        ret = dbfilter.run_command(auto_process=True)
        if ret != 0:
            self.installing = False
            if ret == 3:
                # error already handled by Install
                sys.exit(ret)
            elif (os.WIFSIGNALED(ret) and
                  os.WTERMSIG(ret) in (signal.SIGINT, signal.SIGKILL,
                                       signal.SIGTERM)):
                sys.exit(ret)
            elif os.path.exists('/var/lib/ubiquity/install.trace'):
                tbfile = open('/var/lib/ubiquity/install.trace')
                realtb = tbfile.read()
                tbfile.close()
                raise RuntimeError, ("Install failed with exit code %s\n%s" %
                                     (ret, realtb))
            else:
                raise RuntimeError, ("Install failed with exit code %s; see "
                                     "/var/log/syslog" % ret)

        while self.progress_position.depth() != 0:
            self.debconf_progress_stop()

        # just to make sure
        self.progressDialogue.hide()

        self.installing = False
        quitText = '<qt>%s</qt>' % self.get_string("finished_label")
        rebootButtonText = self.get_string("reboot_button")
        quitButtonText = self.get_string("quit_button")
        titleText = self.get_string("finished_dialog")

        ##FIXME use non-stock messagebox to customise button text
        #quitAnswer = QMessageBox.question(self.userinterface, titleText, quitText, rebootButtonText, quitButtonText)
        self.run_success_cmd()
        if not self.get_reboot_seen():
            if 'UBIQUITY_ONLY' in os.environ:
                quitText = self.get_string('ubiquity/finished_restart_only')
            messageBox = QMessageBox(QMessageBox.Question, titleText, quitText, QMessageBox.NoButton, self.userinterface)
            messageBox.addButton(rebootButtonText, QMessageBox.AcceptRole)
            if not 'UBIQUITY_ONLY' in os.environ:
                messageBox.addButton(quitButtonText, QMessageBox.RejectRole)
            quitAnswer = messageBox.exec_()

            if quitAnswer == 0:
                self.reboot()
        elif self.get_reboot():
            self.reboot()

    def reboot(self, *args):
        """reboot the system after installing process."""

        self.returncode = 10
        self.quit()

    def do_reboot(self):
        """Callback for main program to actually reboot the machine."""

        if 'DESKTOP_SESSION' in os.environ:
            execute('dcop', 'ksmserver', 'ksmserver', 'logout',
                    # ShutdownConfirmNo, ShutdownTypeReboot,
                    # ShutdownModeForceNow
                    '0', '1', '2')
        else:
            execute('reboot')

    def quit(self):
        """quit installer cleanly."""

        # exiting from application
        self.current_page = None
        if self.dbfilter is not None:
            self.dbfilter.cancel_handler()
        self.app.exit()

    def on_cancel_clicked(self):
        warning_dialog_label = self.get_string("warning_dialog_label")
        abortTitle = self.get_string("warning_dialog")
        continueButtonText = self.get_string("continue")
        response = QMessageBox.question(self.userinterface, abortTitle, warning_dialog_label, abortTitle, continueButtonText)
        if response == 0:
            self.current_page = None
            self.quit()
            return True
        else:
            return False

    def info_loop(self, widget):
        """check if all entries from Identification screen are filled."""

        if (widget is not None and widget.objectName() == 'fullname' and
            not self.username_edited):
            self.userinterface.username.blockSignals(True)
            new_username = unicode(widget.text()).split(' ')[0]
            new_username = new_username.encode('ascii', 'ascii_transliterate')
            new_username = new_username.lower()
            self.userinterface.username.setText(new_username)
            self.userinterface.username.blockSignals(False)
        elif (widget is not None and widget.objectName() == 'username' and
              not self.hostname_edited):
            if self.laptop:
                hostname_suffix = '-laptop'
            else:
                hostname_suffix = '-desktop'
            self.userinterface.hostname.blockSignals(True)
            self.userinterface.hostname.setText(unicode(widget.text()) + hostname_suffix)
            self.userinterface.hostname.blockSignals(False)

        complete = True
        for name in ('username', 'password', 'verified_password', 'hostname'):
            if getattr(self.userinterface, name).text() == '':
                complete = False
        self.allow_go_forward(complete)

    def on_username_insert_text(self):
        self.username_edited = (self.userinterface.username.text() != '')

    def on_hostname_insert_text(self):
        self.hostname_edited = (self.userinterface.hostname.text() != '')

    def on_next_clicked(self):
        """Callback to control the installation process between steps."""

        if not self.allowed_change_step or not self.allowed_go_forward:
            return

        self.allow_change_step(False)

        step = self.step_name(self.get_current_page())

        self.userinterface.fullname_error_image.hide()
        self.userinterface.fullname_error_reason.hide()
        self.userinterface.username_error_image.hide()
        self.userinterface.username_error_reason.hide()
        self.userinterface.password_error_image.hide()
        self.userinterface.password_error_reason.hide()
        self.userinterface.hostname_error_image.hide()
        self.userinterface.hostname_error_reason.hide()

        if self.dbfilter is not None:
            self.dbfilter.ok_handler()
            # expect recursive main loops to be exited and
            # debconffilter_done() to be called when the filter exits
        else:
            self.app.exit()

    def on_keyboard_layout_selected(self):
        if isinstance(self.dbfilter, console_setup.ConsoleSetup):
            layout = self.get_keyboard()
            if layout is not None:
                self.current_layout = layout
                self.dbfilter.change_layout(layout)

    def on_keyboard_variant_selected(self):
        if isinstance(self.dbfilter, console_setup.ConsoleSetup):
            layout = self.get_keyboard()
            variant = self.get_keyboard_variant()
            if layout is not None and variant is not None:
                self.dbfilter.apply_keyboard(layout, variant)

    def process_step(self):
        """Process and validate the results of this step."""

        # setting actual step
        step_num = self.get_current_page()
        step = self.step_name(step_num)
        syslog.syslog('Step_before = %s' % step)

        if step.startswith("stepPart"):
            self.previous_partitioning_page = step_num

        # Language
        elif step == "stepLanguage":
            self.translate_widgets()
        # Automatic partitioning
        elif step == "stepPartAuto":
            self.process_autopartitioning()
        # Advanced partitioning
        elif step == "stepPartAdvanced":
            ##if not 'UBIQUITY_MIGRATION_ASSISTANT' in os.environ:  #FIXME for migration-assistant
            self.info_loop(None)
            #else:
            #    self.set_current_page(self.steps.page_num(self.stepMigrationAssistant))
        # Identification
        elif step == "stepUserInfo":
            self.process_identification()

    def process_identification (self):
        """Processing identification step tasks."""

        error_msg = []
        error = 0

        # Validation stuff

        # checking hostname entry
        hostname = self.userinterface.hostname.text()
        for result in validation.check_hostname(unicode(hostname)):
            if result == validation.HOSTNAME_LENGTH:
                error_msg.append("The hostname must be between 1 and 63 characters long.")
            elif result == validation.HOSTNAME_BADCHAR:
                error_msg.append("The hostname may only contain letters, digits, hyphens, and dots.")
            elif result == validation.HOSTNAME_BADHYPHEN:
                error_msg.append("The hostname may not start or end with a hyphen.")
            elif result == validation.HOSTNAME_BADDOTS:
                error_msg.append('The hostname may not start or end with a dot, or contain the sequence "..".')

        # showing warning message is error is set
        if len(error_msg) != 0:
            self.userinterface.hostname_error_reason.setText("\n".join(error_msg))
            self.userinterface.hostname_error_reason.show()
            self.userinterface.hostname_error_image.show()
            self.stay_on_page = True
        else:
            self.stay_on_page = False

    def process_autopartitioning(self):
        """Processing automatic partitioning step tasks."""

        self.app.processEvents()

        # For safety, if we somehow ended up improperly initialised
        # then go to manual partitioning.
        #choice = self.get_autopartition_choice()[0]
        #if self.manual_choice is None or choice == self.manual_choice:
        #    self.set_current_page(WIDGET_STACK_STEPS["stepPartAdvanced"])
        #else:
        #    self.set_current_page(WIDGET_STACK_STEPS["stepUserInfo"])

    def on_back_clicked(self):
        """Callback to set previous screen."""

        if not self.allowed_change_step:
            return

        self.allow_change_step(False)

        self.backup = True

        # Enabling next button
        self.allow_go_forward(True)
        # Setting actual step
        step = self.step_name(self.get_current_page())
        self.userinterface.setCursor(QCursor(Qt.WaitCursor))

        changed_page = False

        if str(step) == "stepReady":
            self.userinterface.next.setText(self.get_string("next").replace('_', '&', 1))
            self.userinterface.next.setIcon(self.forwardIcon)
            self.translate_widget(self.userinterface.next, self.locale)

        if self.dbfilter is not None:
            self.dbfilter.cancel_handler()
            # expect recursive main loops to be exited and
            # debconffilter_done() to be called when the filter exits
        else:
            self.app.exit()

    def selected_language (self):
        selection = self.userinterface.language_treeview.selectedItems()
        if len(selection) == 1:
            value = unicode(selection[0].text())
            return self.language_choice_map[value][1]
        else:
            return ''

    def on_language_treeview_selection_changed (self):
        lang = self.selected_language()
        if lang:
            # strip encoding; we use UTF-8 internally no matter what
            lang = lang.split('.')[0].lower()
            for widget in (self.userinterface, self.userinterface.welcome_heading_label, self.userinterface.welcome_text_label, self.userinterface.oem_id_label, self.userinterface.release_notes_label, self.userinterface.release_notes_frame, self.userinterface.next, self.userinterface.back, self.userinterface.cancel, self.userinterface.step_label):
                self.translate_widget(widget, lang)

    def on_steps_switch_page(self, newPageID):
        self.current_page = newPageID
        self.translate_widget(self.userinterface.step_label, self.locale)
        syslog.syslog('switched to page %s' % self.step_name(newPageID))

    def on_autopartition_toggled (self, choice, enable):
        """Update autopartitioning screen when the resize button is
        selected."""

        if choice in self.autopartition_extras:
            self.autopartition_extras[choice].setEnabled(enable)

    def watch_debconf_fd (self, from_debconf, process_input):
        self.debconf_fd_counter = 0
        self.socketNotifierRead = QSocketNotifier(from_debconf, QSocketNotifier.Read, self.app)
        self.app.connect(self.socketNotifierRead, SIGNAL("activated(int)"), self.watch_debconf_fd_helper_read)

        self.socketNotifierWrite = QSocketNotifier(from_debconf, QSocketNotifier.Write, self.app)
        self.app.connect(self.socketNotifierWrite, SIGNAL("activated(int)"), self.watch_debconf_fd_helper_write)

        self.socketNotifierException = QSocketNotifier(from_debconf, QSocketNotifier.Exception, self.app)
        self.app.connect(self.socketNotifierException, SIGNAL("activated(int)"), self.watch_debconf_fd_helper_exception)

        self.debconf_callbacks[from_debconf] = process_input
        self.current_debconf_fd = from_debconf

    def watch_debconf_fd_helper_read (self, source):
        self.debconf_fd_counter += 1
        debconf_condition = 0
        debconf_condition |= filteredcommand.DEBCONF_IO_IN
        self.debconf_callbacks[source](source, debconf_condition)

    def watch_debconf_fd_helper_write(self, source):
        debconf_condition = 0
        debconf_condition |= filteredcommand.DEBCONF_IO_OUT
        self.debconf_callbacks[source](source, debconf_condition)

    def watch_debconf_fd_helper_exception(self, source):
        debconf_condition = 0
        debconf_condition |= filteredcommand.DEBCONF_IO_ERR
        self.debconf_callbacks[source](source, debconf_condition)

    def debconf_progress_start (self, progress_min, progress_max, progress_title):
        if self.progress_cancelled:
            return False

        if progress_title is None:
            progress_title = ""
        total_steps = progress_max - progress_min
        if self.progressDialogue is None:
            skipText = self.get_string("progress_cancel_button")
            self.progressDialogue = QProgressDialog('', skipText, 0, total_steps, self.userinterface)
            self.progressDialogue.setWindowModality(Qt.WindowModal);
            self.cancelButton = QPushButton(skipText, self.progressDialogue)
            self.progressDialogue.setCancelButton(self.cancelButton)
            # This needs to be called after setCancelButton, otherwise that
            # function will cause the button to be shown again.
            self.cancelButton.hide()
        elif self.progress_position.depth() == 0:
            self.progressDialogue.setMaximum(total_steps)

        self.progress_position.start(progress_min, progress_max,
                                     progress_title)
        self.progressDialogue.setWindowTitle(progress_title)
        self.debconf_progress_set(0)
        self.progressDialogue.setLabel(QLabel(''))
        self.progressDialogue.show()
        return True

    def debconf_progress_set (self, progress_val):
        self.progress_cancelled = self.progressDialogue.wasCanceled()
        if self.progress_cancelled:
            return False
        self.progress_position.set(progress_val)
        fraction = self.progress_position.fraction()
        self.progressDialogue.setValue(
            int(fraction * self.progressDialogue.maximum()))
        return True

    def debconf_progress_step (self, progress_inc):
        self.progress_cancelled = self.progressDialogue.wasCanceled()
        if self.progress_cancelled:
            return False
        self.progress_position.step(progress_inc)
        fraction = self.progress_position.fraction()
        self.progressDialogue.setValue(
            int(fraction * self.progressDialogue.maximum()))
        return True

    def debconf_progress_info (self, progress_info):
        self.progress_cancelled = self.progressDialogue.wasCanceled()
        if self.progress_cancelled:
            return False
        self.progressDialogue.setLabel(QLabel(progress_info))
        return True

    def debconf_progress_stop (self):
        self.progress_cancelled = self.progressDialogue.wasCanceled()
        if self.progress_cancelled:
            self.progress_cancelled = False
            return False
        self.progress_position.stop()
        if self.progress_position.depth() == 0:
            self.progressDialogue.hide()
        else:
            self.progressDialogue.setWindowTitle(self.progress_position.title())
        return True

    def debconf_progress_region (self, region_start, region_end):
        self.progress_position.set_region(region_start, region_end)

    def debconf_progress_cancellable (self, cancellable):
        if cancellable:
            self.cancelButton.show()
        else:
            self.cancelButton.hide()
            self.progress_cancelled = False

    def on_progress_cancel_button_clicked (self, button):
        self.progress_cancelled = True

    def debconffilter_done (self, dbfilter):
        ##FIXME in Qt 4 without this disconnect it calls watch_debconf_fd_helper_read once more causing
        ## a crash after the keyboard stage.  No idea why.
        self.app.disconnect(self.socketNotifierRead, SIGNAL("activated(int)"), self.watch_debconf_fd_helper_read)
        if BaseFrontend.debconffilter_done(self, dbfilter):
            self.app.exit()
            return True
        else:
            return False

    def set_language_choices (self, choices, choice_map):
        BaseFrontend.set_language_choices(self, choices, choice_map)
        self.userinterface.language_treeview.clear()
        for choice in choices:
            QListWidgetItem(QString(unicode(choice)), self.userinterface.language_treeview)

    def set_language (self, language):
        counter = 0
        max = self.userinterface.language_treeview.count()
        while counter < max:
            selection = self.userinterface.language_treeview.item(counter)
            if selection is None:
                value = "C"
            else:
                value = unicode(selection.text())
            if value == language:
                selection.setSelected(True)
                self.userinterface.language_treeview.scrollToItem(selection)
                break
            counter += 1

    def get_language (self):
        items = self.userinterface.language_treeview.selectedItems()
        if len(items) == 1:
            value = unicode(items[0].text())
            return self.language_choice_map[value][0]
        else:
            return 'C'

    def get_oem_id (self):
        return unicode(self.userinterface.oem_id_entry.text())

    def set_timezone (self, timezone):
        self.tzmap.set_tz_from_name(timezone)

    def get_timezone (self):
        return self.tzmap.get_selected_tz_name()

    def set_keyboard_choices(self, choices):
        self.userinterface.keyboardlayoutview.clear()
        for choice in sorted(choices):
            QListWidgetItem(QString(unicode(choice)), self.userinterface.keyboardlayoutview)

        if self.current_layout is not None:
            self.set_keyboard(self.current_layout)

    def set_keyboard (self, layout):
        BaseFrontend.set_keyboard(self, layout)
        counter = 0
        max = self.userinterface.keyboardlayoutview.count()
        while counter < max:
            selection = self.userinterface.keyboardlayoutview.item(counter)
            if unicode(selection.text()) == layout:
                selection.setSelected(True)
                self.userinterface.keyboardlayoutview.scrollToItem(selection)
                break
            counter += 1

    def get_keyboard (self):
        items = self.userinterface.keyboardlayoutview.selectedItems()
        if len(items) == 1:
            return unicode(items[0].text())
        else:
            return None

    def set_keyboard_variant_choices(self, choices):
        self.userinterface.keyboardvariantview.clear()
        for choice in sorted(choices):
            QListWidgetItem(QString(unicode(choice)), self.userinterface.keyboardvariantview)

    def set_keyboard_variant(self, variant):
        counter = 0
        max = self.userinterface.keyboardvariantview.count()
        while counter < max:
            selection = self.userinterface.keyboardvariantview.item(counter)
            if unicode(selection.text()) == variant:
                selection.setSelected(True)
                self.userinterface.keyboardvariantview.scrollToItem(selection)
                break
            counter += 1

    def get_keyboard_variant(self):
        items = self.userinterface.keyboardvariantview.selectedItems()
        if len(items) == 1:
            return unicode(items[0].text())
        else:
            return None

    def set_autopartition_choices (self, choices, extra_options,
                                   resize_choice, manual_choice):
        BaseFrontend.set_autopartition_choices(self, choices, extra_options,
                                               resize_choice, manual_choice)

        children = self.userinterface.autopartition_frame.children()
        for child in children:
            if isinstance(child, QVBoxLayout) or isinstance(child, QButtonGroup):
                pass
            else:
                self.autopartition_vbox.removeWidget(child)
                child.hide()

        firstbutton = None
        idCounter = 0
        for choice in choices:
            button = QRadioButton(choice, self.userinterface.autopartition_frame)
            self.autopartition_buttongroup.addButton(button, idCounter)
            id = self.autopartition_buttongroup.id(button)

            #Qt changes the string by adding accelarators,
            #so keep pristine string here as is returned later to partman
            self.autopartition_buttongroup_texts[id] = choice
            if firstbutton is None:
                firstbutton = button
            self.autopartition_vbox.addWidget(button)

            if choice in extra_options:
                indent_hbox = QHBoxLayout()
                self.autopartition_vbox.addLayout(indent_hbox)
                indent_hbox.addSpacing(10)
                if choice == resize_choice:
                    containerWidget = QWidget(self.userinterface.autopartition_frame)
                    indent_hbox.addWidget(containerWidget)
                    new_size_hbox = QHBoxLayout()
                    containerWidget.setLayout(new_size_hbox)
                    new_size_label = QLabel("New partition size:", self.userinterface.autopartition_frame)
                    new_size_hbox.addWidget(new_size_label)
                    self.translate_widget(new_size_label, self.locale)
                    new_size_hbox.addWidget(new_size_label)
                    new_size_label.show()
                    new_size_scale_vbox = QVBoxLayout()
                    new_size_hbox.addLayout(new_size_scale_vbox)
                    #self.new_size_scale = QSlider(Qt.Horizontal, self.userinterface.autopartition_frame)
                    self.new_size_scale = ResizeWidget(self.userinterface.autopartition_frame)
                    #self.new_size_scale.setMaximum(100)
                    #self.new_size_scale.setSizePolicy(QSizePolicy.Expanding,
                    #                                  QSizePolicy.Minimum)
                    #self.app.connect(self.new_size_scale,
                    #                 SIGNAL("valueChanged(int)"),
                    #                 self.update_new_size_label)
                    new_size_scale_vbox.addWidget(self.new_size_scale)
                    self.new_size_scale.show()

                    # TODO evand 2008-02-12: Until the new resize widget is
                    # ported to Qt, resize_orig_size and resize_path are not
                    # needed.
                    self.resize_min_size, self.resize_max_size, \
                        self.resize_orig_size, self.resize_path = \
                            extra_options[choice]
                    if (self.resize_min_size is not None and
                        self.resize_max_size is not None):
                        min_percent = int(math.ceil(
                            100 * self.resize_min_size / self.resize_max_size))
                        #self.new_size_scale.setMinimum(min_percent)
                        #self.new_size_scale.setMaximum(100)
                        #self.new_size_scale.setValue(
                        #    int((min_percent + 100) / 2))
                        self.new_size_scale.set_part_size(self.resize_orig_size)
                        self.new_size_scale.set_min(self.resize_min_size)
                        self.new_size_scale.set_max(self.resize_max_size)
                        self.new_size_scale.set_device(self.resize_path)
                    self.autopartition_extras[choice] = containerWidget
                elif choice != manual_choice:
                    disk_frame = QFrame(self.userinterface.autopartition_frame)
                    indent_hbox.addWidget(disk_frame)
                    disk_vbox = QVBoxLayout(disk_frame)
                    disk_buttongroup = QButtonGroup(disk_frame)
                    disk_buttongroup_texts = {}
                    extra_firstbutton = None
                    extraIdCounter = 0
                    for extra in extra_options[choice]:
                        if extra == '':
                            disk_vbox.addSpacing(10)
                        else:
                            extra_button = QRadioButton(
                                extra, disk_frame)
                            disk_buttongroup.addButton(extra_button, extraIdCounter)
                            extra_id = disk_buttongroup.id(extra_button)
                            # Qt changes the string by adding accelerators,
                            # so keep the pristine string here to be
                            # returned to partman later.
                            disk_buttongroup_texts[extra_id] = extra
                            if extra_firstbutton is None:
                                extra_firstbutton = extra_button
                            disk_vbox.addWidget(extra_button)
                            extraIdCounter += 1
                    if extra_firstbutton is not None:
                        extra_firstbutton.setChecked(True)
                    self.autopartition_extra_buttongroup[choice] = \
                        disk_buttongroup
                    self.autopartition_extra_buttongroup_texts[choice] = \
                        disk_buttongroup_texts
                    disk_frame.show()
                    self.autopartition_extras[choice] = disk_frame

            def make_on_autopartition_toggled_slot(choice):
                def slot(enable):
                    return self.on_autopartition_toggled(choice, enable)
                return slot

            self.on_autopartition_toggled(choice, button.isChecked())
            self.autopartition_handlers[choice] = \
                make_on_autopartition_toggled_slot(choice)
            self.app.connect(button, SIGNAL('toggled(bool)'),
                             self.autopartition_handlers[choice])

            button.show()
            idCounter += 1
        if firstbutton is not None:
            firstbutton.setChecked(True)

        # make sure we're on the autopartitioning page
        self.set_current_page(WIDGET_STACK_STEPS["stepPartAuto"])

    def get_autopartition_choice (self):
        id = self.autopartition_buttongroup.checkedId()
        choice = unicode(self.autopartition_buttongroup_texts[id])

        if choice == self.resize_choice:
            # resize choice should have been hidden otherwise
            assert self.new_size_scale is not None
            return choice, self.new_size_scale.get_value()
        elif (choice != self.manual_choice and
              choice in self.autopartition_extra_buttongroup):
            disk_id = self.autopartition_extra_buttongroup[choice].checkedId()
            disk_texts = self.autopartition_extra_buttongroup_texts[choice]
            return choice, unicode(disk_texts[disk_id])
        else:
            return choice, None

    def update_partman (self, disk_cache, partition_cache, cache_order):
        #throwing away the old model if there is one
        self.partition_tree_model = PartitionModel(self, self.userinterface.partition_list_treeview)

        children = self.userinterface.partition_bar_frame.children()
        for child in children:
            if isinstance(child, PartitionsBar):
                self.partition_bar_vbox.removeWidget(child)
                child.hide()
                del child

        self.partition_bars = []
        indexCount = -1
        for item in cache_order:
            if item in disk_cache:
                self.partition_tree_model.append([item, disk_cache[item]], self)
                indexCount += 1
                self.partition_bar = PartitionsBar(1000, self.userinterface.partition_bar_frame)
                self.partition_bars.append(self.partition_bar)
                self.partition_bar_vbox.addWidget(self.partition_bar)
            else:
                self.partition_tree_model.append([item, partition_cache[item]], self)
                indexCount += 1
                size = int(partition_cache[item]['parted']['size']) / 1000000000 #GB, MB are too big
                fs = partition_cache[item]['parted']['fs']
                path = partition_cache[item]['parted']['path'].replace("/dev/","")
                if fs == "free":
                    path = fs
                self.partition_bar.addPartition(size, indexCount, fs, path)
        for barSignal in self.partition_bars:
            self.app.connect(barSignal, SIGNAL("clicked(int)"), self.partitionClicked)
            for barSlot in self.partition_bars:
                self.app.connect(barSignal, SIGNAL("clicked(int)"), barSlot.raiseFrames)

        self.userinterface.partition_list_treeview.setModel(self.partition_tree_model)
        self.app.disconnect(self.userinterface.partition_list_treeview.selectionModel(), SIGNAL("selectionChanged(const QItemSelection&, const QItemSelection&)"), self.on_partition_list_treeview_selection_changed)
        self.app.connect(self.userinterface.partition_list_treeview.selectionModel(), SIGNAL("selectionChanged(const QItemSelection&, const QItemSelection&)"), self.on_partition_list_treeview_selection_changed)

        # make sure we're on the advanced partitioning page
        self.set_current_page(WIDGET_STACK_STEPS["stepPartAdvanced"])

    def partitionClicked(self, indexCounter):
        """ a partition in a partition bar has been clicked, select correct entry in list view """
        index = self.partition_tree_model.index(indexCounter,2)
        flags = self.userinterface.partition_list_treeview.selectionCommand(index)
        rect = self.userinterface.partition_list_treeview.visualRect(index)
        self.userinterface.partition_list_treeview.setSelection(rect, flags)

    def partman_create_dialog(self, devpart, partition):
        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return

        self.create_dialog = QDialog(self.userinterface)
        uic.loadUi("%s/partition_create_dialog.ui" % UIDIR, self.create_dialog)
        self.app.connect(self.create_dialog.partition_create_use_combo, SIGNAL("currentIndexChanged(int)"), self.on_partition_create_use_combo_changed)
        self.translate_widget_children(self.create_dialog)

        # TODO cjwatson 2006-11-01: Because partman doesn't use a question
        # group for these, we have to figure out in advance whether each
        # question is going to be asked.

        if partition['parted']['type'] == 'pri/log':
            # Is there already an extended partition?
            for child in self.partition_tree_model.children():
                data = child.itemData
                otherpart = data[1]
                if (otherpart['dev'] == partition['dev'] and
                    'id' in otherpart and
                    otherpart['parted']['type'] == 'logical'):
                    self.create_dialog.partition_create_type_logical.setChecked(True)
                    break
            else:
                self.create_dialog.partition_create_type_primary.setChecked(True)
        else:
            self.create_dialog.partition_create_type_label.hide()
            self.create_dialog.partition_create_type_widget.hide()
        # Yes, I know, 1000000 bytes is annoying. Sorry. This is what
        # partman expects.
        max_size_mb = int(partition['parted']['size']) / 1000000
        self.create_dialog.partition_create_size_spinbutton.setMaximum(max_size_mb)
        self.create_dialog.partition_create_size_spinbutton.setValue(max_size_mb)

        self.create_dialog.partition_create_place_beginning.setChecked(True)

        self.create_use_method_names = {}
        for method, name, description in self.dbfilter.create_use_as():
            self.create_use_method_names[description] = name
            self.create_dialog.partition_create_use_combo.addItem(description)
        if self.create_dialog.partition_create_use_combo.count() == 0:
            self.create_dialog.partition_create_use_combo.setEnabled(False)

        self.create_dialog.partition_create_mount_combo.clear()
        for mp, choice_c, choice in self.dbfilter.default_mountpoint_choices():
            ##FIXME gtk frontend has a nifty way of showing the user readable
            ##'choice' text in the drop down, but only selecting the 'mp' text
            self.create_dialog.partition_create_mount_combo.addItem(mp)
        self.create_dialog.partition_create_mount_combo.clearEditText()

        response = self.create_dialog.exec_()

        if (response == QDialog.Accepted and
            isinstance(self.dbfilter, partman.Partman)):
            if partition['parted']['type'] == 'primary':
                prilog = partman.PARTITION_TYPE_PRIMARY
            elif partition['parted']['type'] == 'logical':
                prilog = partman.PARTITION_TYPE_LOGICAL
            elif partition['parted']['type'] == 'pri/log':
                if self.create_dialog.partition_create_type_primary.isChecked():
                    prilog = partman.PARTITION_TYPE_PRIMARY
                else:
                    prilog = partman.PARTITION_TYPE_LOGICAL

            if self.create_dialog.partition_create_place_beginning.isChecked():
                place = partman.PARTITION_PLACE_BEGINNING
            else:
                place = partman.PARTITION_PLACE_END

            method_description = unicode(self.create_dialog.partition_create_use_combo.currentText())
            method = self.create_use_method_names[method_description]

            mountpoint = str(self.create_dialog.partition_create_mount_combo.currentText())

            self.allow_change_step(False)
            self.dbfilter.create_partition(
                devpart,
                str(self.create_dialog.partition_create_size_spinbutton.value()),
                prilog, place, method, mountpoint)

    def on_partition_create_use_combo_changed (self, combobox):
        if not hasattr(self, 'create_use_method_names'):
            return
        known_filesystems = ('ext3', 'ext2', 'reiserfs', 'jfs', 'xfs',
                             'fat16', 'fat32', 'ntfs')
        text = str(self.create_dialog.partition_create_use_combo.currentText())
        if text not in self.create_use_method_names:
            return
        method = self.create_use_method_names[text]
        if method not in known_filesystems:
            self.create_dialog.partition_create_mount_combo.clearEditText()
            self.create_dialog.partition_create_mount_combo.setEnabled(False)
        else:
            self.create_dialog.partition_create_mount_combo.setEnabled(True)
            if isinstance(self.dbfilter, partman.Partman):
                self.create_dialog.partition_create_mount_combo.clear()
                for mp, choice_c, choice in \
                    self.dbfilter.default_mountpoint_choices(method):
                    self.create_dialog.partition_create_mount_combo.addItem(mp)

    def partman_edit_dialog(self, devpart, partition):
        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return

        self.edit_dialog = QDialog(self.userinterface)
        uic.loadUi("%s/partition_edit_dialog.ui" % UIDIR, self.edit_dialog)
        self.app.connect(self.edit_dialog.partition_edit_use_combo, SIGNAL("currentIndexChanged(int)"), self.on_partition_edit_use_combo_changed)
        self.translate_widget_children(self.edit_dialog)

        current_size = None
        if ('can_resize' not in partition or not partition['can_resize'] or
            'resize_min_size' not in partition or
            'resize_max_size' not in partition):
            self.edit_dialog.partition_edit_size_label.hide()
            self.edit_dialog.partition_edit_size_spinbutton.hide()
        else:
            # Yes, I know, 1000000 bytes is annoying. Sorry. This is what
            # partman expects.
            min_size_mb = int(partition['resize_min_size']) / 1000000
            cur_size_mb = int(partition['parted']['size']) / 1000000
            max_size_mb = int(partition['resize_max_size']) / 1000000
            # Bad things happen if the current size is out of bounds.
            min_size_mb = min(min_size_mb, cur_size_mb)
            max_size_mb = max(cur_size_mb, max_size_mb)
            self.edit_dialog.partition_edit_size_spinbutton.setMinimum(min_size_mb)
            self.edit_dialog.partition_edit_size_spinbutton.setMaximum(max_size_mb)
            self.edit_dialog.partition_edit_size_spinbutton.setSingleStep(1)
            self.edit_dialog.partition_edit_size_spinbutton.setValue(cur_size_mb)

            current_size = str(self.edit_dialog.partition_edit_size_spinbutton.value())

        self.edit_use_method_names = {}
        method_descriptions = {}
        self.edit_dialog.partition_edit_use_combo.clear()
        for script, arg, option in partition['method_choices']:
            self.edit_use_method_names[option] = arg
            method_descriptions[arg] = option
            self.edit_dialog.partition_edit_use_combo.addItem(option)
        current_method = self.dbfilter.get_current_method(partition)
        if current_method and current_method in method_descriptions:
            current_method_description = method_descriptions[current_method]
            index = self.edit_dialog.partition_edit_use_combo.findText(current_method_description)
            self.edit_dialog.partition_edit_use_combo.setCurrentIndex(index)

        if 'id' not in partition:
            self.edit_dialog.partition_edit_format_label.hide()
            self.edit_dialog.partition_edit_format_checkbutton.hide()
            current_format = False
        elif 'method' in partition:
            self.edit_dialog.partition_edit_format_label.show()
            self.edit_dialog.partition_edit_format_checkbutton.show()
            self.edit_dialog.partition_edit_format_checkbutton.setEnabled(
                'can_activate_format' in partition)
            current_format = (partition['method'] == 'format')
        else:
            self.edit_dialog.partition_edit_format_label.show()
            self.edit_dialog.partition_edit_format_checkbutton.show()
            self.edit_dialog.partition_edit_format_checkbutton.setEnabled(False)
            current_format = False
        self.edit_dialog.partition_edit_format_checkbutton.setChecked(
            current_format)

        self.edit_dialog.partition_edit_mount_combo.clear()
        if 'mountpoint_choices' in partition:
            for mp, choice_c, choice in partition['mountpoint_choices']:
                ##FIXME gtk frontend has a nifty way of showing the user readable
                ##'choice' text in the drop down, but only selecting the 'mp' text
                self.edit_dialog.partition_edit_mount_combo.addItem(mp)
        current_mountpoint = self.dbfilter.get_current_mountpoint(partition)
        if current_mountpoint is not None:
            index = self.edit_dialog.partition_edit_mount_combo.findText(current_method)
            if index != -1:
                self.edit_dialog.partition_edit_mount_combo.setCurrentIndex(index)
            else:
                self.edit_dialog.partition_edit_mount_combo.addItem(current_mountpoint)
                self.edit_dialog.partition_edit_mount_combo.setCurrentIndex(self.edit_dialog.partition_edit_mount_combo.count() - 1)

        response = self.edit_dialog.exec_()

        if (response == QDialog.Accepted and
            isinstance(self.dbfilter, partman.Partman)):
            size = None
            if current_size is not None:
                size = str(self.edit_dialog.partition_edit_size_spinbutton.value())

            method_description = unicode(self.edit_dialog.partition_edit_use_combo.currentText())
            method = self.edit_use_method_names[method_description]

            format = self.edit_dialog.partition_edit_format_checkbutton.isChecked()

            mountpoint = str(self.edit_dialog.partition_edit_mount_combo.currentText())

            if (current_size is not None and size is not None and
                current_size == size):
                size = None
            if method == current_method:
                method = None
            if format == current_format:
                format = None
            if mountpoint == current_mountpoint:
                mountpoint = None

            if (size is not None or method is not None or format is not None or
                mountpoint is not None):
                self.allow_change_step(False)
                edits = {'size': size, 'method': method,
                         'mountpoint': mountpoint}
                if format is not None:
                    edits['format'] = 'dummy'
                self.dbfilter.edit_partition(devpart, **edits)

    def on_partition_edit_use_combo_changed(self, combobox):
        if not hasattr(self, 'edit_use_method_names'):
            return
        # If the selected method isn't a filesystem, then selecting a mount
        # point makes no sense. TODO cjwatson 2007-01-31: Unfortunately we
        # have to hardcode the list of known filesystems here.
        known_filesystems = ('ext3', 'ext2', 'reiserfs', 'jfs', 'xfs',
                             'fat16', 'fat32', 'ntfs')
        text = unicode(self.edit_dialog.partition_edit_use_combo.currentText())
        if text not in self.edit_use_method_names:
            return
        method = self.edit_use_method_names[text]
        if method not in known_filesystems:
            self.edit_dialog.partition_edit_mount_combo.clearEditText()
            self.edit_dialog.partition_edit_mount_combo.setEnabled(False)
            self.edit_dialog.partition_edit_format_checkbutton.setEnabled(False)
        else:
            self.edit_dialog.partition_edit_mount_combo.setEnabled(True)
            self.edit_dialog.partition_edit_format_checkbutton.setEnabled(True)
            if isinstance(self.dbfilter, partman.Partman):
                self.edit_dialog.partition_edit_mount_combo.clear()
                for mp, choice_c, choice in \
                    self.dbfilter.default_mountpoint_choices(method):
                    self.edit_dialog.partition_edit_mount_combo.addItem(mp)

    def on_partition_list_treeview_selection_changed(self, selected, deselected):
        self.userinterface.partition_button_new_label.setEnabled(False)
        self.userinterface.partition_button_new.setEnabled(False)
        self.userinterface.partition_button_edit.setEnabled(False)
        self.userinterface.partition_button_delete.setEnabled(False)
        if not isinstance(self.dbfilter, partman.Partman):
            return

        indexes = self.userinterface.partition_list_treeview.selectedIndexes()
        if indexes:
            index = indexes[0]
            for bar in self.partition_bars:
                ##bar.selected(index)  ##FIXME find out row from index and call bar.selected on it
                bar.raiseFrames()
            item = index.internalPointer()
            devpart = item.itemData[0]
            partition = item.itemData[1]
        else:
            devpart = None
            partition = None

        for action in self.dbfilter.get_actions(devpart, partition):
            if action == 'new_label':
                self.userinterface.partition_button_new_label.setEnabled(True)
            elif action == 'new':
                self.userinterface.partition_button_new.setEnabled(True)
            elif action == 'edit':
                self.userinterface.partition_button_edit.setEnabled(True)
            elif action == 'delete':
                self.userinterface.partition_button_delete.setEnabled(True)
        self.userinterface.partition_button_undo.setEnabled(True)

    def on_partition_list_treeview_activated(self, index):
        if not self.allowed_change_step:
            return
        item = index.internalPointer()
        devpart = item.itemData[0]
        partition = item.itemData[1]

        if 'id' not in partition:
            # Are there already partitions on this disk? If so, don't allow
            # activating the row to offer to create a new partition table,
            # to avoid mishaps.
            for child in self.partition_tree_model.children():
                data = child.itemData
                otherpart = data[1]
                if otherpart['dev'] == partition['dev'] and 'id' in otherpart:
                    break
            else:
                if not isinstance(self.dbfilter, partman.Partman):
                    return
                self.allow_change_step(False)
                self.dbfilter.create_label(devpart)
        elif partition['parted']['fs'] == 'free':
            if 'can_new' in partition and partition['can_new']:
                self.partman_create_dialog(devpart, partition)
        else:
            self.partman_edit_dialog(devpart, partition)

    def on_partition_list_new_label_activate(self, ticked):
        selected = self.userinterface.partition_list_treeview.selectedIndexes()
        if not selected:
            return
        index = selected[0]
        item = index.internalPointer()
        devpart = item.itemData[0]

        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return
        self.allow_change_step(False)
        self.dbfilter.create_label(devpart)

    def on_partition_list_new_activate(self, ticked):
        selected = self.userinterface.partition_list_treeview.selectedIndexes()
        if not selected:
            return
        index = selected[0]
        item = index.internalPointer()
        devpart = item.itemData[0]
        partition = item.itemData[1]
        self.partman_create_dialog(devpart, partition)

    def on_partition_list_edit_activate(self, ticked):
        selected = self.userinterface.partition_list_treeview.selectedIndexes()
        if not selected:
            return
        index = selected[0]
        item = index.internalPointer()
        devpart = item.itemData[0]
        partition = item.itemData[1]
        self.partman_edit_dialog(devpart, partition)

    def on_partition_list_delete_activate(self, ticked):
        selected = self.userinterface.partition_list_treeview.selectedIndexes()
        if not selected:
            return
        index = selected[0]
        item = index.internalPointer()
        devpart = item.itemData[0]

        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return
        self.allow_change_step(False)
        self.dbfilter.delete_partition(devpart)

    def on_partition_list_undo_activate(self, ticked):
        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return
        self.allow_change_step(False)
        self.dbfilter.undo()

    def partman_popup (self, position):
        if not self.allowed_change_step:
            return
        if not isinstance(self.dbfilter, partman.Partman):
            return

        selected = self.userinterface.partition_list_treeview.selectedIndexes()
        if selected:
            index = selected[0]
            item = index.internalPointer()
            devpart = item.itemData[0]
            partition = item.itemData[1]
        else:
            devpart = None
            partition = None

        #partition_list_menu = gtk.Menu()
        partition_list_menu = QMenu(self.userinterface)
        for action in self.dbfilter.get_actions(devpart, partition):
            if action == 'new_label':
                new_label_item = partition_list_menu.addAction(
                    self.get_string('partition_button_new_label'))
                self.app.connect(new_label_item, SIGNAL("triggered(bool)"),
                                 self.on_partition_list_new_label_activate)
            elif action == 'new':
                new_item = partition_list_menu.addAction(
                    self.get_string('partition_button_new'))
                self.app.connect(new_item, SIGNAL("triggered(bool)"),
                                 self.on_partition_list_new_activate)
            elif action == 'edit':
                edit_item = partition_list_menu.addAction(
                    self.get_string('partition_button_edit'))
                self.app.connect(edit_item, SIGNAL("triggered(bool)"),
                                 self.on_partition_list_edit_activate)
            elif action == 'delete':
                delete_item = partition_list_menu.addAction(
                    self.get_string('partition_button_delete'))
                self.app.connect(delete_item, SIGNAL("triggered(bool)"),
                                 self.on_partition_list_delete_activate)
        if partition_list_menu.children():
            partition_list_menu.addSeparator()
        undo_item = partition_list_menu.addAction(
            self.get_string('partman/text/undo_everything'))
        self.app.connect(undo_item, SIGNAL("triggered(bool)"),
                         self.on_partition_list_undo_activate)

        partition_list_menu.exec_(QCursor.pos())

    def set_fullname(self, value):
        self.userinterface.fullname.setText(unicode(value, "UTF-8"))

    def get_fullname(self):
        return unicode(self.userinterface.fullname.text())

    def set_username(self, value):
        self.userinterface.username.setText(unicode(value, "UTF-8"))

    def get_username(self):
        return unicode(self.userinterface.username.text())

    def get_password(self):
        return unicode(self.userinterface.password.text())

    def get_verified_password(self):
        return unicode(self.userinterface.verified_password.text())

    def username_error(self, msg):
        self.userinterface.username_error_reason.setText(msg)
        self.userinterface.username_error_image.show()
        self.userinterface.username_error_reason.show()

    def password_error(self, msg):
        self.userinterface.password_error_reason.setText(msg)
        self.userinterface.password_error_image.show()
        self.userinterface.password_error_reason.show()

    def get_hostname (self):
        return unicode(self.userinterface.hostname.text())

    def set_summary_text (self, text):
        i = text.find("\n")
        while i != -1:
            text = text[:i] + "<br>" + text[i+1:]
            i = text.find("\n")
        self.userinterface.ready_text.setText(text)

    def set_grub_combo(self, options):
        # TODO evand 2008-02-13: Port grub combobox.
        pass

    def on_advanced_button_clicked (self):
        self.translate_widget_children(self.advanceddialog)
        self.app.connect(self.advanceddialog.grub_enable, SIGNAL("stateChanged(int)"), self.toggle_grub)
        self.app.connect(self.advanceddialog.proxy_host_entry, SIGNAL("textChanged(const QString &)"), self.enable_proxy_spinbutton)
        display = False
        summary_device = self.get_summary_device()
        grub_en = self.get_grub()
        if summary_device is not None:
            display = True
            self.advanceddialog.bootloader_group_label.show()
            self.advanceddialog.grub_device_label.show()
            self.advanceddialog.grub_device_entry.show()
            self.advanceddialog.grub_device_entry.setText(summary_device)
            self.advanceddialog.grub_device_entry.setEnabled(grub_en)
            self.advanceddialog.grub_device_label.setEnabled(grub_en)
        else:
            self.advanceddialog.bootloader_group_label.hide()
            self.advanceddialog.grub_device_label.hide()
            self.advanceddialog.grub_device_entry.hide()
        if self.popcon is not None:
            display = True
            self.advanceddialog.popcon_group_label.show()
            self.advanceddialog.popcon_checkbutton.show()
            self.advanceddialog.popcon_checkbutton.setChecked(self.popcon)
        else:
            self.advanceddialog.popcon_group_label.hide()
            self.advanceddialog.popcon_checkbutton.hide()

        display = True
        if self.http_proxy_host:
            self.advanceddialog.proxy_port_spinbutton.setEnabled(True)
            self.advanceddialog.proxy_host_entry.setText(unicode(self.http_proxy_host))
        else:
            self.advanceddialog.proxy_port_spinbutton.setEnabled(False)
        self.advanceddialog.proxy_port_spinbutton.setValue(self.http_proxy_port)

        if not display:
            return

        response = self.advanceddialog.exec_()
        if response == QDialog.Accepted:
            self.set_summary_device(
                unicode(self.advanceddialog.grub_device_entry.text()))
            self.set_popcon(self.advanceddialog.popcon_checkbutton.isChecked())
            self.set_grub(self.advanceddialog.grub_enable.isChecked())
            self.set_proxy_host(unicode(self.advanceddialog.proxy_host_entry.text()))
            self.set_proxy_port(self.advanceddialog.proxy_port_spinbutton.value())

    def enable_proxy_spinbutton(self):
        self.advanceddialog.proxy_port_spinbutton.setEnabled(self.advanceddialog.proxy_host_entry.text() != '')

    def toggle_grub(self):
        grub_en = self.advanceddialog.grub_enable.isChecked()
        self.advanceddialog.grub_device_entry.setEnabled(grub_en)
        self.advanceddialog.grub_device_label.setEnabled(grub_en)

    def return_to_partitioning (self):
        """If the install progress bar is up but still at the partitioning
        stage, then errors can safely return us to partitioning.
        """
        if self.installing and not self.installing_no_return:
            # Go back to the partitioner and try again.
            #self.live_installer.show()
            self.pagesindex = 1
            self.dbfilter = partman.Partman(self)
            self.set_current_page(self.previous_partitioning_page)
            self.userinterface.next.setText(self.get_string("next").replace('_', '&', 1))
            self.userinterface.next.setIcon(self.forwardIcon)
            self.translate_widget(self.userinterface.next, self.locale)
            self.backup = True
            self.installing = False

    def error_dialog (self, title, msg, fatal=True):
        self.run_automation_error_cmd()
        self.allow_change_step(True)
        # TODO: cancel button as well if capb backup
        QMessageBox.warning(self.userinterface, title, msg, QMessageBox.Ok)
        if fatal:
            self.return_to_partitioning()

    def question_dialog (self, title, msg, options, use_templates=True):
        self.run_automation_error_cmd()
        # I doubt we'll ever need more than three buttons.
        assert len(options) <= 3, options

        self.allow_change_step(True)
        buttons = {}
        messageBox = QMessageBox(QMessageBox.Question, title, msg, QMessageBox.NoButton, self.userinterface)
        for option in options:
            if use_templates:
                text = self.get_string(option)
            else:
                text = option
            if text is None:
                text = option
            # Convention for options is to have the affirmative action last; KDE
            # convention is to have it first.
            if option == options[-1]:
                button = messageBox.addButton(text, QMessageBox.AcceptRole)
            else:
                button = messageBox.addButton(text, QMessageBox.RejectRole)
            buttons[button] = option

        response = messageBox.exec_()

        if response < 0:
            return None
        else:
            return buttons[messageBox.clickedButton()]

    def refresh (self):
        self.app.processEvents()

    # Run the UI's main loop until it returns control to us.
    def run_main_loop (self):
        self.allow_change_step(True)
        #self.app.exec_()   ##FIXME Qt 4 won't allow nested main loops, here it just returns directly
        self.mainLoopRunning = True
        while self.mainLoopRunning:    # nasty, but works OK
            self.app.processEvents()

    # Return control to the next level up.
    def quit_main_loop (self):
        #self.app.exit()
        self.mainLoopRunning = False

    # returns the current wizard page
    def get_current_page(self):
      return self.userinterface.widgetStack.indexOf(self.userinterface.widgetStack.currentWidget())

    def on_fullname_changed(self):
        self.info_loop(self.userinterface.fullname)

    def on_username_changed(self):
        self.info_loop(self.userinterface.username)

    def on_password_changed(self):
        self.info_loop(self.userinterface.password)

    def on_verified_password_changed(self):
        self.info_loop(self.userinterface.verified_password)

    def on_hostname_changed(self):
        self.info_loop(self.userinterface.hostname)

    def update_new_size_label(self, value):
        if self.new_size_value is None:
            return
        if self.resize_max_size is not None:
            size = value * self.resize_max_size / 100
            text = '%d%% (%s)' % (value, format_size(size))
        else:
            text = '%d%%' % value
        self.new_size_value.setText(text)

    def quit(self):
        """quit installer cleanly."""

        # exiting from application
        self.current_page = None
        if self.dbfilter is not None:
            self.dbfilter.cancel_handler()
        self.app.exit()

class TimezoneMap(object):
    def __init__(self, frontend):
        self.frontend = frontend
        self.tzdb = ubiquity.tz.Database()
        #self.tzmap = ubiquity.emap.EMap()
        self.tzmap = MapWidget(self.frontend.userinterface.map_frame)
        self.frontend.map_vbox.addWidget(self.tzmap)
        self.tzmap.show()
        self.update_timeout = None
        self.point_selected = None
        self.point_hover = None
        self.location_selected = None

        timezone_city_combo = self.frontend.userinterface.timezone_city_combo
        self.timezone_city_index = {}  #map human readable city name to Europe/London style zone
        self.city_index = []  # map cities to indexes for the combo box

        prev_continent = ''
        for location in self.tzdb.locations:
            #self.tzmap.add_point("", location.longitude, location.latitude,
            #                     NORMAL_RGBA)
            zone_bits = location.zone.split('/')
            if len(zone_bits) == 1:
                continue
            continent = zone_bits[0]
            if continent != prev_continent:
                timezone_city_combo.addItem('')
                self.city_index.append('')
                timezone_city_combo.addItem("--- %s ---" % continent)
                self.city_index.append("--- %s ---" % continent)
                prev_continent = continent
            human_zone = '/'.join(zone_bits[1:]).replace('_', ' ')
            timezone_city_combo.addItem(human_zone)
            self.timezone_city_index[human_zone] = location.zone
            self.city_index.append(human_zone)
            self.tzmap.cities[human_zone] = [location.latitude, location.longitude]

        self.frontend.app.connect(self.tzmap, SIGNAL("cityChanged"), self.cityChanged)
        self.mapped()

    def set_city_text(self, name):
        """ Gets a long name, Europe/London """
        timezone_city_combo = self.frontend.userinterface.timezone_city_combo
        count = timezone_city_combo.count()
        found = False
        i = 0
        zone_bits = name.split('/')
        human_zone = '/'.join(zone_bits[1:]).replace('_', ' ')
        while not found and i < count:
            if str(timezone_city_combo.itemText(i)) == human_zone:
                timezone_city_combo.setCurrentIndex(i)
                found = True
            i += 1

    def set_zone_text(self, location):
        offset = location.utc_offset
        if offset >= datetime.timedelta(0):
            minuteoffset = int(offset.seconds / 60)
        else:
            minuteoffset = int(offset.seconds / 60 - 1440)
        if location.zone_letters == 'GMT':
            text = location.zone_letters
        else:
            text = "%s (GMT%+d:%02d)" % (location.zone_letters,
                                         minuteoffset / 60, minuteoffset % 60)
        self.frontend.userinterface.timezone_zone_text.setText(text)
        translations = gettext.translation('iso_3166',
                                           languages=[self.frontend.locale],
                                           fallback=True)
        self.frontend.userinterface.timezone_country_text.setText(translations.ugettext(location.human_country))
        self.update_current_time()

    def update_current_time(self):
        if self.location_selected is not None:
            try:
                now = datetime.datetime.now(self.location_selected.info)
                self.frontend.userinterface.timezone_time_text.setText(unicode(now.strftime('%X'), "utf-8"))
            except ValueError:
                # Some versions of Python have problems with clocks set
                # before the epoch (http://python.org/sf/1646728).
                self.frontend.userinterface.timezone_time_text.setText('<clock error>')

    def set_tz_from_name(self, name):
        """ Gets a long name, Europe/London """

        (longitude, latitude) = (0.0, 0.0)

        for location in self.tzdb.locations:
            if location.zone == name:
                (longitude, latitude) = (location.longitude, location.latitude)
                break
        else:
            return

        self.location_selected = location
        self.set_city_text(self.location_selected.zone)
        self.set_zone_text(self.location_selected)
        self.frontend.allow_go_forward(True)

        if name == None or name == "":
            return

    def get_tz_from_name(self, name):
        if len(name) != 0 and name in self.timezone_city_index:
            return self.timezone_city_index[name]
        else:
            return None

    def city_combo_changed(self, index):
        city = str(self.frontend.userinterface.timezone_city_combo.currentText())
        try:
            zone = self.timezone_city_index[city]
        except KeyError:
            return
        self.set_tz_from_name(zone)

    def get_selected_tz_name(self):
        name = str(self.frontend.userinterface.timezone_city_combo.currentText())
        return self.get_tz_from_name(name)

    def timeout(self):
        self.update_current_time()
        return True

    def mapped(self):
        if self.update_timeout is None:
            self.update_timeout = QTimer()
            self.frontend.app.connect(self.update_timeout, SIGNAL("timeout()"), self.timeout)
            self.update_timeout.start(100)

    def cityChanged(self):
        self.frontend.userinterface.timezone_city_combo.setCurrentIndex(self.city_index.index(self.tzmap.city))
        self.city_combo_changed(self.frontend.userinterface.timezone_city_combo.currentIndex())
        self.frontend.allow_go_forward(True)

class CityIndicator(QLabel):
    def __init__(self, parent, name="cityindicator"):
        QLabel.__init__(self, parent)
        self.setMouseTracking(True)
        self.setMargin(1)
        self.setIndent(0)
        self.setAutoFillBackground(True)
        self.setLineWidth(1)
        self.setFrameStyle(QFrame.Box | QFrame.Plain)
        self.setPalette(QToolTip.palette())
        self.setText("CityIndicator")

    def mouseMoveEvent(self, mouseEvent):
        mouseEvent.ignore()

    def setText(self, text):
        """ implement auto resize """
        QLabel.setText(self, text)
        self.adjustSize()

class MapWidget(QWidget):
    def __init__(self, parent, name="mapwidget"):
        QWidget.__init__(self, parent)
        self.setObjectName(name)
        self.setAutoFillBackground(True)
        self.imagePath = "/usr/share/ubiquity/pixmaps/world_map-960.png"
        image = QImage(self.imagePath);
        pixmapUnscaled = QPixmap(self.imagePath);
        pixmap = pixmapUnscaled.scaled( QSize(self.width(), self.height()) )
        palette = QPalette()
        palette.setBrush(self.backgroundRole(), QBrush(pixmap))
        self.setPalette(palette)
        self.cities = {}
        self.cities['Edinburgh'] = [self.coordinate(False, 55, 50, 0), self.coordinate(True, 3, 15, 0)]
        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL("timeout()"), self.updateCityIndicator)
        self.setMouseTracking(True)

        self.cityIndicator = CityIndicator(self)
        self.cityIndicator.setText("")
        self.cityIndicator.hide()

    def paintEvent(self, paintEvent):
        ##FIXME this is slow, need to buffer the output.
        painter = QPainter(self)
        for city in self.cities:
            self.drawCity(self.cities[city][0], self.cities[city][1], painter)

    def drawCity(self, lat, long, painter):
        point = self.getPosition(lat, long, self.width(), self.height())
        painter.setPen(QPen(QColor(250,100,100), 1))
        painter.drawPoint(point.x(), point.y()-1)
        painter.drawPoint(point.x()-1, point.y())
        painter.drawPoint(point.x(), point.y())
        painter.drawPoint(point.x()+1, point.y())
        painter.drawPoint(point.x(), point.y()+1)
        painter.setPen(QPen(QColor(0,0,0), 1))
        painter.drawPoint(point.x(), point.y()-2)
        painter.drawPoint(point.x()-1, point.y()-1)
        painter.drawPoint(point.x()+1, point.y()-1)
        painter.drawPoint(point.x()-2, point.y())
        painter.drawPoint(point.x()+2, point.y())
        painter.drawPoint(point.x()-1, point.y()+1)
        painter.drawPoint(point.x()+1, point.y()+1)
        painter.drawPoint(point.x(), point.y()+2)

    def getPosition(self, la, lo, w, h):
        x = (w * (180.0 + lo) / 360.0)
        y = (h * (90.0 - la) / 180.0)

        return QPoint(int(x),int(y))

    def coordinate(self, neg, d, m, s):
        if neg:
            return - (d + m/60.0 + s/3600.0)
        else :
            return d + m/60.0 + s/3600.0

    def getNearestCity(self, w, h, x, y):
        result = None
        dist = 1.0e10
        for city in self.cities:
            pos = self.getPosition(self.cities[city][0], self.cities[city][1], self.width(), self.height())

            d = (pos.x()-x)*(pos.x()-x) + (pos.y()-y)*(pos.y()-y)
            if d < dist:
                dist = d
                self.where = pos
                result = city
        return result

    def mouseMoveEvent(self, mouseEvent):
        self.x = mouseEvent.pos().x()
        self.y = mouseEvent.pos().y()
        if not self.timer.isActive():
            self.timer.setSingleShot(True)
            self.timer.start(25)

    def updateCityIndicator(self):
        city = self.getNearestCity(self.width(), self.height(), self.x, self.y)
        if city is None:
            return
        self.cityIndicator.setText(city)
        movePoint = self.getPosition(self.cities[city][0], self.cities[city][1], self.width(), self.height())
        self.cityIndicator.move(movePoint.x(), movePoint.y() - self.cityIndicator.height())
        self.cityIndicator.show()

    def mouseReleaseEvent(self, mouseEvent):
        pos = mouseEvent.pos()

        city = self.getNearestCity(self.width(), self.height(), pos.x(), pos.y());
        if city is None:
            return
        elif city == "Edinburgh":
            self.city = "London"
        else:
            self.city = city
        self.emit(SIGNAL("cityChanged"), ())

    def resizeEvent(self, resizeEvent):
        image = QImage(self.imagePath);
        pixmapUnscaled = QPixmap(self.imagePath);
        pixmap = pixmapUnscaled.scaled( QSize(self.width(), self.height()), Qt.IgnoreAspectRatio, Qt.SmoothTransformation)
        palette = QPalette()
        palette.setBrush(self.backgroundRole(), QBrush(pixmap))
        self.setPalette(palette)

class PartitionModel(QAbstractItemModel):
    def __init__(self, ubiquity, parent=None):
        QAbstractItemModel.__init__(self, parent)

        rootData = []
        rootData.append(QVariant(ubiquity.get_string('partition_column_device')))
        rootData.append(QVariant(ubiquity.get_string('partition_column_type')))
        rootData.append(QVariant(ubiquity.get_string('partition_column_mountpoint')))
        rootData.append(QVariant(ubiquity.get_string('partition_column_format')))
        rootData.append(QVariant(ubiquity.get_string('partition_column_size')))
        rootData.append(QVariant(ubiquity.get_string('partition_column_used')))
        self.rootItem = TreeItem(rootData)

    def append(self, data, ubiquity):
        self.rootItem.appendChild(TreeItem(data, ubiquity, self.rootItem))

    def columnCount(self, parent):
        if parent.isValid():
            return parent.internalPointer().columnCount()
        else:
            return self.rootItem.columnCount()

    def data(self, index, role):
        if not index.isValid():
            return QVariant()

        item = index.internalPointer()

        if role == Qt.CheckStateRole and index.column() == 3:
            return QVariant(item.data(index.column()))
        elif role == Qt.DisplayRole and index.column() != 3:
            return QVariant(item.data(index.column()))
        else:
            return QVariant()

    def setData(self, index, value, role):
        item = index.internalPointer()
        if role == Qt.CheckStateRole and index.column() == 3:
            item.partman_column_format_toggled(value.toBool())
        self.emit(SIGNAL("dataChanged(const QModelIndex&, const QModelIndex&)"), index, index)
        return True

    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled

        #self.setData(index, QVariant(Qt.Checked), Qt.CheckStateRole)
        #return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        if index.column() == 3:
            item = index.internalPointer()
            if item.formatEnabled():
                return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsUserCheckable
            else:
                return Qt.ItemIsSelectable | Qt.ItemIsUserCheckable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self.rootItem.data(section)

        return QVariant()

    def index(self, row, column, parent = QModelIndex()):
        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.child(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QModelIndex()

    def parent(self, index):
        if not index.isValid():
            return QModelIndex()

        childItem = index.internalPointer()
        parentItem = childItem.parent()

        if parentItem == self.rootItem:
            return QModelIndex()

        return self.createIndex(parentItem.row(), 0, parentItem)

    def rowCount(self, parent):
        if not parent.isValid():
            parentItem = self.rootItem
        else:
            parentItem = parent.internalPointer()

        return parentItem.childCount()

    def children(self):
        return self.rootItem.children()

class TreeItem:
    def __init__(self, data, ubiquity=None, parent=None):
        self.parentItem = parent
        self.itemData = data
        self.childItems = []
        self.ubiquity = ubiquity

    def appendChild(self, item):
        self.childItems.append(item)

    def child(self, row):
        return self.childItems[row]

    def childCount(self):
        return len(self.childItems)

    def children(self):
        return self.childItems

    def columnCount(self):
        if self.parentItem is None:
            return len(self.itemData)
        else:
            return 5

    def data(self, column):
        if self.parentItem is None:
            return QVariant(self.itemData[column])
        elif column == 0:
            return QVariant(self.partman_column_name())
        elif column == 1:
            return QVariant(self.partman_column_type())
        elif column == 2:
            return QVariant(self.partman_column_mountpoint())
        elif column == 3:
            return QVariant(self.partman_column_format())
        elif column == 4:
            return QVariant(self.partman_column_size())
        elif column == 5:
            return QVariant(self.partman_column_used())
        else:
            return QVariant("other")

    def parent(self):
        return self.parentItem

    def row(self):
        if self.parentItem:
            return self.parentItem.childItems.index(self)

        return 0

    def partman_column_name(self):
        partition = self.itemData[1]
        if 'id' not in partition:
            # whole disk
            return partition['device']
        elif partition['parted']['fs'] != 'free':
            return '  %s' % partition['parted']['path']
        elif partition['parted']['type'] == 'unusable':
            return '  %s' % self.ubiquity.get_string('partman/text/unusable')
        else:
            # partman uses "FREE SPACE" which feels a bit too SHOUTY for
            # this interface.
            return '  %s' % self.ubiquity.get_string('partition_free_space')

    def partman_column_type(self):
        partition = self.itemData[1]
        if 'id' not in partition or 'method' not in partition:
            if ('parted' in partition and
                partition['parted']['fs'] != 'free' and
                'detected_filesystem' in partition):
                return partition['detected_filesystem']
            else:
                return ''
        elif ('filesystem' in partition and
              partition['method'] in ('format', 'keep')):
            return partition['acting_filesystem']
        else:
            return partition['method']

    def partman_column_mountpoint(self):
        partition = self.itemData[1]
        if isinstance(self.ubiquity.dbfilter, partman.Partman):
            mountpoint = self.ubiquity.dbfilter.get_current_mountpoint(partition)
            if mountpoint is None:
                mountpoint = ''
        else:
            mountpoint = ''
        return mountpoint

    def partman_column_format(self):
        partition = self.itemData[1]
        if 'id' not in partition:
            return ''
            #cell.set_property('visible', False)
            #cell.set_property('active', False)
            #cell.set_property('activatable', False)
        elif 'method' in partition:
            if partition['method'] == 'format':
                return Qt.Checked
            else:
                return Qt.Unchecked
            #cell.set_property('visible', True)
            #cell.set_property('active', partition['method'] == 'format')
            #cell.set_property('activatable', 'can_activate_format' in partition)
        else:
            return Qt.Unchecked  ##FIXME should be enabled(False)
            #cell.set_property('visible', True)
            #cell.set_property('active', False)
            #cell.set_property('activatable', False)

    def formatEnabled(self):
        """is the format tickbox enabled"""
        partition = self.itemData[1]
        return 'method' in partition and 'can_activate_format' in partition

    def partman_column_format_toggled(self, value):
        if not self.ubiquity.allowed_change_step:
            return
        if not isinstance(self.ubiquity.dbfilter, partman.Partman):
            return
        #model = user_data
        #devpart = model[path][0]
        #partition = model[path][1]
        devpart = self.itemData[0]
        partition = self.itemData[1]
        if 'id' not in partition or 'method' not in partition:
            return
        self.ubiquity.allow_change_step(False)
        self.ubiquity.dbfilter.edit_partition(devpart, format='dummy')

    def partman_column_size(self):
        partition = self.itemData[1]
        if 'id' not in partition:
            return ''
        else:
            # Yes, I know, 1000000 bytes is annoying. Sorry. This is what
            # partman expects.
            size_mb = int(partition['parted']['size']) / 1000000
            return '%d MB' % size_mb

    def partman_column_used(self):
        partition = self.itemData[1]
        if 'id' not in partition or partition['parted']['fs'] == 'free':
            return ''
        elif 'resize_min_size' not in partition:
            return self.ubiquity.get_string('partition_used_unknown')
        else:
            # Yes, I know, 1000000 bytes is annoying. Sorry. This is what
            # partman expects.
            size_mb = int(partition['resize_min_size']) / 1000000
            return '%d MB' % size_mb

#TODO much of this is duplicated from gtk_ui, abstract it
class ResizeWidget(QWidget):
    def __init__(self, parent=None):
        QWidget.__init__(self, parent)

        frame = QFrame(self)
        layout = QHBoxLayout(self)
        layout.addWidget(frame)

        frame.setLineWidth(1)
        frame.setFrameShadow(QFrame.Plain)
        frame.setFrameShape(QFrame.StyledPanel)

        layout = QHBoxLayout(frame)
        layout.setMargin(2)
        splitter = QSplitter(frame)
        splitter.setChildrenCollapsible(False)
        layout.addWidget(splitter)

        self.old_os = QFrame(splitter)
        self.old_os.setLineWidth(1)
        self.old_os.setFrameShadow(QFrame.Raised)
        self.old_os.setFrameShape(QFrame.Box)
        layout = QHBoxLayout(self.old_os)
        self.old_os_label = QLabel(self.old_os)
        layout.addWidget(self.old_os_label)

        self.new_os = QFrame(splitter)
        self.new_os.setLineWidth(1)
        self.new_os.setFrameShadow(QFrame.Raised)
        self.new_os.setFrameShape(QFrame.Box)
        layout = QHBoxLayout(self.new_os)
        self.new_os_label = QLabel(self.new_os)
        layout.addWidget(self.new_os_label)

        self.old_os_label.setAlignment(Qt.AlignHCenter)
        self.new_os_label.setAlignment(Qt.AlignHCenter)

        self.old_os.setAutoFillBackground(True)
        palette = self.old_os.palette()
        palette.setColor(QPalette.Active, QPalette.Background, QColor("#FFA500"))
        palette.setColor(QPalette.Inactive, QPalette.Background, QColor("#FFA500"))

        self.new_os.setAutoFillBackground(True)
        palette = self.new_os.palette()
        palette.setColor(QPalette.Active, QPalette.Background, Qt.white)
        palette.setColor(QPalette.Inactive, QPalette.Background, Qt.white)

        self.part_size = 0
        self.old_os_title = ''
        self._set_new_os_title()
        self.max_size = 0
        self.min_size = 0

    def paintEvent(self, event):
        self._update_min()
        self._update_max()

        s1 = self.old_os.width()
        s2 = self.new_os.width()
        total = s1 + s2

        percent = (float(s1) / float(total))
        txt = '%s\n%.0f%% (%s)' % (self.old_os_title,
            (percent * 100.0),
            format_size(percent * self.part_size))
        self.old_os_label.setText(txt)
        self.old_os.setToolTip(txt)

        percent = (float(s2) / float(total))
        txt = '%s\n%.0f%% (%s)' % (self.new_os_title,
            (percent * 100.0),
            format_size(percent * self.part_size))
        self.new_os_label.setText(txt)
        self.new_os.setToolTip(txt)

    def set_min(self, size):
        self.min_size = size

    def set_max(self, size):
        self.max_size = size

    def set_part_size(self, size):
        self.part_size = size

    def _update_min(self):
        total = self.new_os.width() + self.old_os.width()
        # The minimum percent needs to be 1% greater than the value debconf
        # feeds us, otherwise the resize will fail.
        tmp = (self.min_size / self.part_size) + 0.01
        pixels = int(tmp * total)
        self.old_os.setMinimumWidth(pixels)

    def _update_max(self):
        total = self.new_os.width() + self.old_os.width()
        tmp = ((self.part_size - self.max_size) / self.part_size)
        pixels = int(tmp * total)
        self.new_os.setMinimumWidth(pixels)

    def _set_new_os_title(self):
        self.new_os_title = ''
        fp = None
        try:
            fp = open('/cdrom/.disk/info')
            line = fp.readline()
            if line:
                self.new_os_title = ' '.join(line.split()[:2])
        except:
            syslog.syslog(syslog.LOG_ERR,
                "Unable to determine the distribution name from /cdrom/.disk/info")
        finally:
            if fp is not None:
                fp.close()
        if not self.new_os_title:
            self.new_os_title = 'Kubuntu'

    def set_device(self, dev):
        '''Sets the title of the old partition to the name found in os_prober.
           On failure, sets the title to the device name or the empty string.'''
        if dev:
            self.old_os_title = find_in_os_prober(dev)
        if dev and not self.old_os_title:
            self.old_os_title = dev
        elif not self.old_os_title:
            self.old_os_title = ''
     
    def get_value(self):
        '''Returns the percent the old partition is of the maximum size it can be.'''
        s1 = self.old_os.width()
        s2 = self.new_os.width()
        totalwidth = s1 + s2
        percentwidth = float(s1) / float(totalwidth)
        percentpart = percentwidth * self.part_size
        return int((percentpart / self.max_size) * 100)
