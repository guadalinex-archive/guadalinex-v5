# -*- coding: utf-8 -*-

import apt_pkg
import subprocess
import os

from utils.sudo import get_sudo
from gettext import gettext as _

class Synaptic(object):

    def __init__(self):
        apt_pkg.init()

    def install(self, pkg_list):
        if not get_sudo():
            return False

        cmd = ["/usr/bin/sudo", "/usr/sbin/synaptic", "--hide-main-window",  "--non-interactive"]

        cmd.append("--set-selections")
        cmd.append("--progress-str")
        cmd.append(_("Installing packages"))
        cmd.append("--finish-str")
        cmd.append(_("Installed packages"))
        proc = subprocess.Popen(cmd, stdin = subprocess.PIPE)
        print proc
        f = proc.stdin

	try:
            for pkg in pkg_list:
                f.write("%s\tinstall\n" % pkg)
            f.close()
	except:
	    return False

        proc.wait()
        return True


    def check(self, pkg_list):
        """
        Return True if all packages in pkg_list are installed. False in other
        case.
        """
        #Collect the packages by name
        packages = apt_pkg.GetCache().Packages
        packages_dict = {}
        for pkg in packages:
            packages_dict[pkg.Name] = pkg

        #Check state
        for pkg_name in pkg_list:
            if (not packages_dict.has_key(pkg_name)) or \
                    (not packages_dict[pkg_name].CurrentVer):
                return False
        return True


if __name__ == "__main__":
    s = Synaptic()
    s.install(['ifrench'])
