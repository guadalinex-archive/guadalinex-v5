#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import pwd
import re
import subprocess
import syslog

def find_in_os_prober(device):
    '''Look for the device name in the output of os-prober.
       Returns the friendly name of the device, or the empty string on error.'''
    os.seteuid(0)
    try:
        if not find_in_os_prober.oslist:
            subp = subprocess.Popen(['os-prober'], stdout=subprocess.PIPE,
                stderr=subprocess.PIPE)
            result = subp.communicate()[0].splitlines()
            for res in result:
                res = res.split(':')
                find_in_os_prober.oslist[res[0]] = res[1]
        return find_in_os_prober.oslist[device]
    except Exception, e:
        syslog.syslog(syslog.LOG_ERR,
            "Error in find_in_os_prober: %s" % str(e))
        return ''
    finally:
        drop_privileges()
find_in_os_prober.oslist = {}

def execute(*args):
    """runs args* in shell mode. Output status is taken."""

    log_args = ['log-output', '-t', 'ubiquity']
    log_args.extend(args)

    try:
        status = subprocess.call(log_args)
    except IOError, e:
        syslog.syslog(syslog.LOG_ERR, ' '.join(log_args))
        syslog.syslog(syslog.LOG_ERR,
                      "OS error(%s): %s" % (e.errno, e.strerror))
        return False
    else:
        if status != 0:
            syslog.syslog(syslog.LOG_ERR, ' '.join(log_args))
            return False
        syslog.syslog(' '.join(log_args))
        return True

def execute_root(*args):
    os.seteuid(0)
    execute(*args)
    drop_privileges()

def format_size(size):
    """Format a partition size."""
    if size < 1024:
        unit = 'B'
        factor = 1
    elif size < 1024 * 1024:
        unit = 'kB'
        factor = 1024
    elif size < 1024 * 1024 * 1024:
        unit = 'MB'
        factor = 1024 * 1024
    elif size < 1024 * 1024 * 1024 * 1024:
        unit = 'GB'
        factor = 1024 * 1024 * 1024
    else:
        unit = 'TB'
        factor = 1024 * 1024 * 1024 * 1024
    return '%.1f %s' % (float(size) / factor, unit)

def drop_all_privileges():
    # gconf needs both the UID and effective UID set.
    if 'SUDO_GID' in os.environ:
        gid = int(os.environ['SUDO_GID'])
        os.setregid(gid, gid)
    if 'SUDO_UID' in os.environ:
        uid = int(os.environ['SUDO_UID'])
        os.setreuid(uid, uid)
        os.environ['HOME'] = pwd.getpwuid(uid).pw_dir

def drop_privileges():
    if 'SUDO_GID' in os.environ:
        gid = int(os.environ['SUDO_GID'])
        os.setegid(gid)
    if 'SUDO_UID' in os.environ:
        uid = int(os.environ['SUDO_UID'])
        os.seteuid(uid)

def debconf_escape(text):
    escaped = text.replace('\\', '\\\\').replace('\n', '\\n')
    return re.sub(r'(\s)', r'\\\1', escaped)

# vim:ai:et:sts=4:tw=80:sw=4:
