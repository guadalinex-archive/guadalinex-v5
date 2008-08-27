#!/usr/bin/env python

import shutil
import os

DEFAULT_SESSION = '/usr/share/gnome/default.session'

if __name__ == '__main__':
    if not os.path.exists(DEFAULT_SESSION):
	exit(0)

    # do a backup of the original session
    backup_file = DEFAULT_SESSION + '.before-eadmin'
    if not os.path.exists(backup_file):
        shutil.copyfile(DEFAULT_SESSION, backup_file)

    lines = file(DEFAULT_SESSION).readlines()

    # modify the session
    out = file(DEFAULT_SESSION, 'w')
    for line in lines:
        if line.startswith('num_clients'):
            num_clients = int(line.strip().split('=')[1])
            line = 'num_clients=%d\n' % (num_clients + 1)
        out.write(line)

    out.write('%d,id=default%d\n' % (num_clients, num_clients))
    out.write('%d,Priority=70\n' % num_clients)
    cmd = '/usr/bin/certmanager.py --search-path=~/certificados --run-only-once --sm-client-id default%d' % num_clients
    out.write('%d,RestartCommand=%s\n' % (num_clients, cmd))
