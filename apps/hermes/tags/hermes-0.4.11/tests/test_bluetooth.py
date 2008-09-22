#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys
sys.path.append('../')

import unittest
import actors

from actortest import ActorTest
from dev_bluetooth import PROPERTIES
from gettext import gettext as _

class BluetoothActorTest(ActorTest):

    def setUp(self):
        ActorTest.setUp(self)
        print _("Configuring:"), PROPERTIES

    def test_add_actor_from_properties(self):
        """Testing DeviceListener.add_actor_from_propertites
        """

        actor = self.devicelistener.add_actor_from_properties(PROPERTIES)
        self.failUnless(isinstance(actor, actors.bluetooth.Actor))


    def test_on_added(self):
        print _("Adding:"), PROPERTIES['info.udi']
        ActorTest.devicelistener.on_device_added(PROPERTIES['info.udi'])


if __name__ == '__main__':
    unittest.main()
