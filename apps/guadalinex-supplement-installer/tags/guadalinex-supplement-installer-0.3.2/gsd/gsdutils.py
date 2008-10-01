#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import logging

APTCONFPATH='/usr/share/gsd/apt.conf'
SOURCESFILE='/tmp/gsd/sources.list'
DISTRONAME='toro'

class SupplementCustomizer(object):

    def __init__(self, mountpoint, use_xwindow = True,
            as_user = False):
        self.logger = logging.getLogger()
        self.mountpoint = mountpoint
        self.use_xwindow = use_xwindow
        self.as_user = as_user


    def customize(self):
        """
        This method install suppletory.
        """
        self.__prepare_system() 
        self.apt_get_update()


    def apt_get_update(self):
        #Update apt system
        cmd = 'APT_CONFIG=%s ' % APTCONFPATH  
        if not self.as_user:
            cmd += 'sudo synaptic --hide-main-window' 
            cmd += ' --update-at-startup --non-interactive'
        else:
            cmd += 'apt-get update'
        os.system(cmd)


    def get_diskdefines(self ):
        filepath = self.mountpoint + \
                '/README.diskdefines'
        try:
            fileobject = open(filepath)
        except Exception, e:
            self.logger.error(str(e))
            return {}

        result = []
        for line in fileobject.readlines():
            items = line.split(None, 2)
            try:
                result.append((items[1],items[2]))
            except IndexError, e:
                result.append((items[1], ''))

        return result


    def __create_sources_list(self):
        self.logger.debug('Creating sources.list')
        diskdefines = self.get_diskdefines()
        fileobj = open(SOURCESFILE, 'w')

        #Create entries for the supplement's URIs
        for key, value in diskdefines:
            if key.startswith('URI'):
                self.__process_uri(value, fileobj)
        fileobj.close()


    def __process_uri(self, value, fileobj):
        self.logger.debug('Processing uri: ' + value)
        if value.startswith('http://') or \
                value.startswith('ftp://'):
            print "<#> http: ", value
            fileobj.write('deb ' + str(value) + '\n')

        else:
            fileobj.write('deb file:' + self.mountpoint + value + \
                    ' '+ DISTRONAME +' main \n')


    def __prepare_system(self):
        #Try for password. Three times.
        res = 768 
        attemps = 0

        # Errno 768: Bad password
        while res == 768 and attemps < 3:
            if not self.as_user:
                res = os.system('gksudo -m "Introduzca contraseña" /bin/true')
            else:
                res = 1
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
        self.__create_sources_list()
         
