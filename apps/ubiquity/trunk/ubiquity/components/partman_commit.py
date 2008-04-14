# -*- coding: UTF-8 -*-

# Copyright (C) 2006, 2007 Canonical Ltd.
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

import debconf

from ubiquity.filteredcommand import FilteredCommand
from ubiquity.parted_server import PartedServer

class PartmanCommit(FilteredCommand):
    def __init__(self, frontend=None):
        FilteredCommand.__init__(self, frontend)

    def prepare(self):
        questions = ['^partman/confirm.*',
                     'type:boolean',
                     'ERROR',
                     'PROGRESS']
        return ('/bin/partman-commit', questions,
                {'PARTMAN_ALREADY_CHECKED': '1'})

    def error(self, priority, question):
        self.frontend.error_dialog(self.description(question),
                                   self.extended_description(question))
        self.succeeded = False
        # Unlike a normal error handler, we want to force exit.
        self.done = True
        return True

    def run(self, priority, question):
        if self.done:
            return self.succeeded

        if question.startswith('partman/confirm'):
            if question == 'partman/confirm':
                self.db.set('ubiquity/partman-made-changes', 'true')
            else:
                self.db.set('ubiquity/partman-made-changes', 'false')
            self.preseed(question, 'true')
            return True

        elif self.question_type(question) == 'boolean':
            response = self.frontend.question_dialog(
                self.description(question),
                self.extended_description(question),
                ('ubiquity/text/go_back', 'ubiquity/text/continue'))

            answer_reversed = False
            if (question == 'partman-jfs/jfs_boot' or
                question == 'partman-jfs/jfs_root'):
                answer_reversed = True
            if response is None or response == 'ubiquity/text/continue':
                answer = answer_reversed
            else:
                answer = not answer_reversed
                self.succeeded = False
                self.done = True
                self.frontend.return_to_partitioning()
            if answer:
                self.preseed(question, 'true')
            else:
                self.preseed(question, 'false')
            return True

        else:
            return FilteredCommand.run(self, priority, question)
