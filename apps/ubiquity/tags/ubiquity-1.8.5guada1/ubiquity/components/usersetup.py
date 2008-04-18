# -*- coding: UTF-8 -*-

# Copyright (C) 2005 Canonical Ltd.
# Written by Colin Watson <cjwatson@ubuntu.com>.
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
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from ubiquity.filteredcommand import FilteredCommand

class UserSetup(FilteredCommand):
    def prepare(self):
        if self.frontend.get_fullname() == '':
            fullname = self.db.get('passwd/user-fullname')
            if fullname != '':
                self.frontend.set_fullname(fullname)
        if self.frontend.get_username() == '':
            username = self.db.get('passwd/username')
            if username != '':
                self.frontend.set_username(username)

        questions = ['^passwd/user-fullname$', '^passwd/username$',
                     '^passwd/user-password$', '^passwd/user-password-again$',
                     'ERROR']
        return (['/usr/lib/ubiquity/user-setup/user-setup-ask', '/target'],
                questions)

    def set(self, question, value):
        if question == 'passwd/username':
            if self.frontend.get_username() != '':
                self.frontend.set_username(value)

    def ok_handler(self):
        fullname = self.frontend.get_fullname()
        username = self.frontend.get_username()
        password = self.frontend.get_password()
        password_confirm = self.frontend.get_verified_password()

        self.preseed('passwd/user-fullname', fullname)
        self.preseed('passwd/username', username)
        # TODO: maybe encrypt these first
        self.preseed('passwd/user-password', password)
        self.preseed('passwd/user-password-again', password_confirm)
        if self.frontend.oem_config:
            self.preseed('passwd/user-uid', '29999')
        else:
            self.preseed('passwd/user-uid', '')

        FilteredCommand.ok_handler(self)

    def error(self, priority, question):
        if question.startswith('passwd/username-'):
            self.frontend.username_error(self.extended_description(question))
        elif question.startswith('user-setup/password-'):
            self.frontend.password_error(self.extended_description(question))
        else:
            self.frontend.error_dialog(self.description(question),
                                       self.extended_description(question))
        return FilteredCommand.error(self, priority, question)
