# -*- coding: utf-8 -*-

import apt_pkg
import subprocess
import os
import os.path

from utils.sudo import get_sudo
from gettext import gettext as _


class IPkgInstaller(object):
    """
    Common interface for packages installers.
    """

    def check(self, pkg_list):
        """  Check packages installation.

        @rtype: boolean
        @return: True if _all_ packages in pkg_list  re installed. 
                 False otherwise.
        """
        raise NotImplementedError


    def install(self, pkg_list):
        """ Install all packages in pkg_list.

        @rtype: boolean
        @return: True if success. False otherwise.
        """
        raise NotImplementedError





class PkgInstaller(IPkgInstaller):

    def __init__(self):
        # Select correct PkgInstaller
        config_path = os.path.abspath('actors/config/installer')
        installer_name = open(config_path).read().strip()
        if installer_name == 'synaptic':
            self.pkg_installer = SynapticInstaller()
        elif installer_name == 'rpm':
            self.pkg_installer = RpmInstaller()
        else:
            self.pkg_installer = SynapticInstaller()


    def check(self, pkg_list):
        return  self.pkg_installer.check(pkg_list)

    
    def install(self, pkg_list):
        return  self.pkg_installer.install(pkg_list)


class SynapticInstaller(IPkgInstaller):

    def __init__(self):
        apt_pkg.init()

    def install(self, pkg_list):
        if not get_sudo():
            return False

        cmd = ["/usr/bin/sudo", "/usr/sbin/synaptic", 
                "--hide-main-window",  "--non-interactive"]

        cmd.append("--set-selections")
        cmd.append("--progress-str")
        cmd.append(_("Installing packages"))
        cmd.append("--finish-str")
        cmd.append(_("Installed packages"))
        proc = subprocess.Popen(cmd, stdin = subprocess.PIPE)
        print proc
        f = proc.stdin

        for pkg in pkg_list:
            f.write("%s\tinstall\n" % pkg)
        f.close()
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


class RpmInstaller(IPkgInstaller):
    pass


if __name__ == "__main__":
    s = SynapticInstaller()
    s.check(['ifrench'])
    s.install(['ifrench'])
