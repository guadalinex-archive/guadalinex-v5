#!/usr/bin/python
# -*- coding: utf-8 -*-

import os

class UsbGrepMap(object):

    DEFAULTMAPFILE = '/etc/hotplug/usb/libsane.usermap'

    def get_module(self, vendorid, productid, 
            mapfile = DEFAULTMAPFILE):

        """
        Returns the module name for device with vendorid and productid.

        @param vendorid     Vendor Id in hexadecimal.
        @param productid    Product Id in hexadecimal.
        """

        vendorid = hex(vendorid)
        productid = hex(productid)

        command = "grepmap --usbmap "
        command += "--file=%s %s %s 0 0 0 0 0 0 0" % \
                (mapfile, vendorid, productid)

        return os.popen(command).read().strip()




        


