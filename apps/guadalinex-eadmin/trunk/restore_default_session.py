#!/usr/bin/env python

import shutil
import os

DEFAULT_SESSION = '/usr/share/gnome/default.session'

if __name__ == '__main__':
    # restore the original session
    backup_file = DEFAULT_SESSION + '.before-eadmin'
    if os.path.exists(backup_file):
        shutil.move(backup_file, DEFAULT_SESSION)
