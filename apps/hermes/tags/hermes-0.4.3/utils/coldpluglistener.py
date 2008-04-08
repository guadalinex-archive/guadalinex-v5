#!/usr/bin/python
# -*- coding: utf8 -*-

import thread
import time
import logging

from utils import DeviceList

class ColdPlugListener:

    def __init__(self, devicelistener):
        self.devicelistener = devicelistener
        self.thread = None
        self.logger = logging.getLogger()
        
    def start(self):
        self.thread = thread.start_new_thread(self.__run, ())
        self.logger.debug("Starting coldplug thread")

    def __run(self):
        self.logger.info("ColdPlugListener thread started")
        dl = DeviceList()

        for ele in dl.get_added():
            try:
                self.logger.debug("Coldplug: DeviceAdded: " + str(ele[0]))
                self.devicelistener.on_device_added(ele[0]) #ele[0] contains the
                                                            #device udi
                time.sleep(0.5)
            except Exception, e:
                self.logger.warning(str(e))

        for ele in dl.get_removed():
            try:
                udi = ele[0]
                properties = ele[1]
                self.devicelistener.get_actor_from_properties(properties)
                self.logger.debug("Coldplug: DeviceRemoved: " + str(udi))
                self.devicelistener.on_device_removed(udi)
                time.sleep(0.5)
            except Exception, e:
                self.logger.warning(str(e))

        dl.save()
