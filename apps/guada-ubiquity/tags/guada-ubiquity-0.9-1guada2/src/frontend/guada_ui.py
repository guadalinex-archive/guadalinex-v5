# -*- coding: utf-8 -*-
#
# «guada-ui» - GuadaLinex user interface
#
# Copyright (C) 2005,2008 Junta de Andalucía
# Copyright (C) 2005, 2006, 2007 Canonical Ltd.
# 
# Authors:
#
# - Original gtk-ui.py that this is based upon:
#   - Javier Carranza <javier.carranza#interactors._coop>
#   - Juan Jesús Ojeda Croissier <juanje#interactors._coop>
#   - Antonio Olmo Titos <aolmo#emergya._info>
#   - Gumer Coronel Pérez <gcoronel#emergya._info>
#   - Colin Watson <cjwatson@ubuntu.com>
#   - Evan Dandrea <evand@ubuntu.com>
#   - Mario Limonciello <superm1@ubuntu.com>
#
# - Guada-ui :
#   - Roberto Majadas <roberto.majadas#openshine._com>
#
#
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
import os
import datetime
import subprocess
import math
import traceback
import syslog
import atexit
import signal
import xml.sax.saxutils
import gettext

import syslog

import pygtk
pygtk.require('2.0')
import pango
import gobject
import gtk.glade

import debconf

from ubiquity import filteredcommand, gconftool, i18n, osextras, validation, \
                     zoommap
from ubiquity.misc import *
from ubiquity.components import console_setup, language, timezone, usersetup, \
                                partman, partman_commit, \
                                summary, install, migrationassistant

import ubiquity.emap
import ubiquity.tz
import ubiquity.progressposition
from ubiquity.frontend.base import BaseFrontend

#Import gtk_ui frontend
import ubiquity.frontend.gtk_ui

# Define global path
PATH = '/usr/share/ubiquity'

BREADCRUMB_STEPS = {
    "stepPartAuto": 1,
    "stepPartAdvanced": 1,
    "stepUserInfo": 2,
    "stepMigrationAssistant": 3,
    "stepReady": 4
}
BREADCRUMB_MAX_STEP = 4


# Define what pages of the UI we want to load.  Note that most of these pages
# are required for the install to complete successfully.
SUBPAGES = [
    "stepGuadaWelcome",
    "stepGuadaPrePartition",
    "stepLanguage",
    "stepLocation",
    "stepKeyboardConf",
    "stepPartAuto",
    "stepPartAdvanced",
    "stepUserInfo",
    "stepMigrationAssistant",
    "stepReady"
]

ubiquity.frontend.gtk_ui.BREADCRUMB_STEPS = BREADCRUMB_STEPS
ubiquity.frontend.gtk_ui.BREADCRUMB_MAX_STEP = BREADCRUMB_MAX_STEP
ubiquity.frontend.gtk_ui.SUBPAGES = SUBPAGES

from subprocess import Popen, PIPE

from ubiquity.filteredcommand import FilteredCommand

try:
    from ubiquity.DiskPreview.DiskPreview import DiskPreview
except:
    syslog.syslog("No pude cargar DiskPreview")

## Ubiquity preseed ##
PRESEED = ["debconf debconf/language string es",
"d-i debian-installer/locale string es_ES.UTF-8",
"d-i clock-setup/utc boolean false",
"d-i time/zone string Europe/Madrid",
"d-i console-setup/modelcode string pc105",
"d-i console-setup/layoutcode string es",
"ubiquity languagechooser/language-name-fb select Spanish",
"ubiquity languagechooser/language-name select Spanish",
"ubiquity languagechooser/language-name-ascii select Spanish",
"ubiquity countrychooser/shortlist select ES",
"ubiquity countrychooser/countryname select Spain",
"ubiquity countrychooser/country-name string Spain",
"ubiquity console-keymaps-at/keymap select es",
"ubiquity localechooser/supported-locales multiselect es_ES.UTF-8",
"ubiquity tzconfig/gmt boolean false",
"ubiquity time/zone select Europe/Madrid",
"console-setup console-setup/variant select Spain",
"console-setup console-setup/layout select Spain"
]

class GuadaPrePartition(FilteredCommand):
    def prepare(self):
        syslog.syslog("------->prepare guadaPrePartition")
        self.preseed('guada-ubiquity/prepartition', 'false')
        questions = ["^guada-ubiquity/prepartition"]
        env = {}
        self.frontend.diskpreview.mount_filesystems()
        return (['/usr/share/ubiquity/guada-prepartition'], questions, env)

    def run(self, priority, question):
        syslog.syslog("-------> run guadaPrePartition")
        if question.startswith('guada-ubiquity/prepartition'):
            #advanced = self.frontend.get_advanced()
            #self.preseed_bool('mythbuntu/advanced_install', advanced)
            print "question" 

        return FilteredCommand.run(self, priority, question)
    
    def ok_handler(self):
        syslog.syslog("------->ok handler guadaPrePartition")
        return FilteredCommand.ok_handler(self)

    def cleanup(self):
        self.frontend.diskpreview.umount_filesystems()
        syslog.syslog("------>cleanup guadaPrePartition")
        return

    
class Wizard(ubiquity.frontend.gtk_ui.Wizard):
    def __init__(self, distro):
        del os.environ['UBIQUITY_MIGRATION_ASSISTANT']
        self.preseed_debconf()
        
        ubiquity.frontend.gtk_ui.Wizard.__init__(self,distro)

    def preseed_debconf(self):
        seed = ""
        for x in PRESEED :
            seed = seed + x + '\n'

        os.system("echo -e '%s' | debconf-set-selections" % seed)

    def show_intro(self):
        ## self.intro_label.set_markup("Bienvenido a Guadalinex")
        self.welcome_image.set_from_file("/usr/share/guada-ubiquity/pics/photo_1024.jpg")
        return True

    def prepartition_intro(self):
        #self.prepartition_image.set_from_file("/usr/share/guada-ubiquity/pics/photo_1024.jpg")
        return True

    def launch_hermes(self):
	os.spawnlp(os.P_NOWAIT,'hermeshardware','hermeshardware')
 
    def run(self):
        """run the interface."""

        if os.getuid() != 0:
            title = ('This installer must be run with administrative '
                     'privileges, and cannot continue without them.')
            dialog = gtk.MessageDialog(self.live_installer, gtk.DIALOG_MODAL,
                                       gtk.MESSAGE_ERROR, gtk.BUTTONS_CLOSE,
                                       title)
            dialog.run()
            sys.exit(1)

	# we need to kill hermeshardware
	os.spawnlp(os.P_NOWAIT,'killall', 'killall', '-9', 'hermes_hardware.py')
	atexit.register(self.launch_hermes)

        self.disable_volume_manager()

        # show interface
        got_intro = self.show_intro()
        self.allow_change_step(True)

        # Guada prepartition page
        self.prepartition_intro()
        
        # Declare SignalHandler
        self.glade.signal_autoconnect(self)

        syslog.syslog("init diskpreview")
        self.diskpreview = DiskPreview()
        syslog.syslog("end init diskpreview")

        self.disk_preview_area.add(self.diskpreview)
        self.diskpreview.show_all()

        # Some signals need to be connected by hand so that we have the
        # handler ids.
        self.username_changed_id = self.username.connect(
            'changed', self.on_username_changed)
        self.hostname_changed_id = self.hostname.connect(
            'changed', self.on_hostname_changed)

        if 'UBIQUITY_MIGRATION_ASSISTANT' in os.environ:
            self.pages = [GuadaPrePartition, partman.Partman,
                usersetup.UserSetup, migrationassistant.MigrationAssistant,
                summary.Summary]
            ## self.pages = [console_setup.ConsoleSetup, partman.Partman,
            ##     usersetup.UserSetup, migrationassistant.MigrationAssistant,
            ##     summary.Summary]
        else:
            self.pages = [GuadaPrePartition, partman.Partman,
                          usersetup.UserSetup, summary.Summary]
            ## self.pages = [console_setup.ConsoleSetup, partman.Partman,
            ##               usersetup.UserSetup, summary.Summary]
            
        self.pagesindex = 0
        pageslen = len(self.pages)
        
        if 'UBIQUITY_AUTOMATIC' in os.environ:
            got_intro = False
            self.debconf_progress_start(0, pageslen,
                self.get_string('ubiquity/install/checking'))
            self.refresh()

        # Start the interface
        # Add GuadaWelcome
        
        global BREADCRUMB_STEPS, BREADCRUMB_MAX_STEP
        for step in BREADCRUMB_STEPS:
            BREADCRUMB_STEPS[step] += 2
        BREADCRUMB_STEPS["stepGuadaWelcome"] = 1
        BREADCRUMB_STEPS["stepGuadaPrePartition"] = 2
        BREADCRUMB_MAX_STEP += 2
        ubiquity.frontend.gtk_ui.BREADCRUMB_STEPS = BREADCRUMB_STEPS
        ubiquity.frontend.gtk_ui.BREADCRUMB_MAX_STEP = BREADCRUMB_MAX_STEP

        syslog.syslog ("%s %s" % (BREADCRUMB_STEPS,BREADCRUMB_MAX_STEP) )
        
        first_step = self.stepGuadaWelcome
        
        self.set_current_page(self.steps.page_num(first_step))
        if got_intro:
            # intro_label was the only focusable widget, but got can-focus
            # removed, so we end up with no input focus and thus pressing
            # Enter doesn't activate the default widget. Work around this.
            self.next.grab_focus()

        if not 'UBIQUITY_MIGRATION_ASSISTANT' in os.environ:
            self.steps.remove_page(self.steps.page_num(self.stepMigrationAssistant))
            for step in BREADCRUMB_STEPS:
                if (BREADCRUMB_STEPS[step] >
                    BREADCRUMB_STEPS["stepMigrationAssistant"]):
                    BREADCRUMB_STEPS[step] -= 1
            BREADCRUMB_MAX_STEP -= 1

        if got_intro:
            gtk.main()

        syslog.syslog("----> RUN : Salte la intro")
        
        while(self.pagesindex < pageslen):
            syslog.syslog("----> RUN : Dentro del while %s" % self.pagesindex)
            if self.current_page == None:
                break

            old_dbfilter = self.dbfilter
            self.dbfilter = self.pages[self.pagesindex](self)

            # Non-debconf steps are no longer possible as the interface is now
            # driven by whether there is a question to ask.
            syslog.syslog("----> RUN : self.dbfilter %s" % self.dbfilter)
            if self.dbfilter is not None and self.dbfilter != old_dbfilter:
                self.allow_change_step(False)
                syslog.syslog("----> RUN : START start dbfilter")
                self.dbfilter.start(auto_process=True)
                syslog.syslog("----> RUN : END start dbfilter")
            gtk.main()
            syslog.syslog("----> RUN : Sali del gtk_main")
            
            if self.backup or self.dbfilter_handle_status():
                if self.installing:
                    syslog.syslog("----> RUN : START progress loop")
                    self.progress_loop()
                    syslog.syslog("----> RUN : END progress loop")
                elif self.current_page is not None and not self.backup:
                    syslog.syslog("----> RUN : START process step")
                    self.process_step()
                    syslog.syslog("----> RUN : END process step")
                    if not self.stay_on_page:
                        self.pagesindex = self.pagesindex + 1
                    if 'UBIQUITY_AUTOMATIC' in os.environ:
                        # if no debconf_progress, create another one, set start to pageindex
                        self.debconf_progress_step(1)
                        self.refresh()
                if self.backup:
                    if self.pagesindex > 0:
                        step = self.step_name(self.steps.get_current_page())
                        if not step == 'stepPartAdvanced':
                            self.pagesindex = self.pagesindex - 1

            while gtk.events_pending():
                gtk.main_iteration()

            # needed to be here for --automatic as there might not be any
            # current page in the event all of the questions have been
            # preseeded.
            if self.pagesindex == pageslen:
                # Ready to install
                self.live_installer.hide()
                self.current_page = None
                self.installing = True
                self.progress_loop()
        return self.returncode

    def set_page(self, n):
        syslog.syslog("--------> set page : %s" % n)
        self.run_automation_error_cmd()
        # We only stop the backup process when we're on a page where questions
        # need to be asked, otherwise you wont be able to back up past
        # migration-assistant.
        self.backup = False
        self.live_installer.show()

        if n == 'Partman':
            # Rather than try to guess which partman page we should be on,
            # we leave that decision to set_autopartitioning_choices and
            # update_partman.
            return
        elif n == 'GuadaPrePartition':
            cur = self.stepGuadaPrePartition
        elif n == 'UserSetup':
            cur = self.stepUserInfo
        elif n == 'Summary':
            cur = self.stepReady
            self.next.set_label(self.get_string('install_button'))
        elif n == 'MigrationAssistant':
            cur = self.stepMigrationAssistant
        else:
            print >>sys.stderr, 'No page found for %s' % n
            return
        
        self.set_current_page(self.steps.page_num(cur))
        if not self.first_seen_page:
            self.first_seen_page = n
        if self.first_seen_page == self.pages[self.pagesindex].__name__:
            self.back.hide()
        elif 'UBIQUITY_AUTOMATIC' not in os.environ:
            self.back.show()

    def process_step(self):
        """Process and validate the results of this step."""

        # setting actual step
        step_num = self.steps.get_current_page()
        step = self.step_name(step_num)

        #Figure out if this is a mythbuntu specific step
        if step == "stepGuadaWelcome":            
            self.process_prepartition()
        else:
            ubiquity.frontend.gtk_ui.Wizard.process_step(self)

    def process_prepartition(self):
        syslog.syslog("-----------> process_prepartition")
        self.allow_go_forward(False)
