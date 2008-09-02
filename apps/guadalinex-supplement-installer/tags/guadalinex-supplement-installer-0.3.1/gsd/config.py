#!/usr/bin/python
# -*- coding: utf-8 -*-

GLVALIDLABELS = [
    "Guadalinex.suppletory.disk",
    "Guadalinex.Suppletory.Disk",
    "GSD-"
    ]


def is_valid_label(label):
    for valid_label in GLVALIDLABELS:
        if label.startswith(valid_label):
            return True

    return False


def prepare_system(self):
    #Try for password. Three times.
    res = 768 
    attemps = 0

    # Errno 768: Bad password
    while res == 768 and attemps < 3:
        res = os.system('gksudo -m "Introduzca contraseña" /bin/true')
        # Errno 512: User press cancel
        if res == 512:
            self.logger.debug("User press cancel")
            return
        attemps += 1

    if res == 768:
        self.logger.debug("Three attemps for password")
        return

    #Prepare apt system
    os.system('cp -a /usr/share/gsd /tmp')

    #Generate sources.list


def guadalinex_suppletory_summoner(mountpoint):
    """
    This method install supplement
    """
    self.__prepare_system() 

    #Update apt system
    cmd = 'APT_CONFIG=' + APTCONFPATH + ' sudo synaptic --hide-main-window' 
    cmd += ' --update-at-startup --non-interactive'
    os.system(cmd)

    #Exec app-install
    os.system('APT_CONFIG=%s sudo guadalinex-app-install %s' % \
            (APTCONFPATH, mountpoint ))
