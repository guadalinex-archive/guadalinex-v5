#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import logging
from gettext import gettext as _

def get_sudo():
    """
    Return true if user has login as sudoer. False in other way.
    """
    logger = logging.getLogger() 
    #Try for password. Three times.
    res = 768 
    attemps = 0

    # Errno 768: Bad password
    while res == 768 and attemps < 3:
	# FIXME: i18n
        res = os.system('gksudo -m "%s" /bin/true' % _('Type password'))
        # Errno 512: User press cancel
        if res == 512:
            logger.debug("User pressed cancel")
            return False
        attemps += 1

    if res == 768:
        logger.debug("Three attemps for password")
        return False

    return True


if __name__ == '__main__':
    get_sudo()
