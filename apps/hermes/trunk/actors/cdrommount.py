#!/usr/bin/python
# -*- coding: utf-8 -*-

#Módulo cdrommount- Módulo que implementa el "actor hardware" para solucionar 
#un problema de montaje de algunos CDROMs y DVDs en Guadalinex v4.1
#
#Copyright (C) 2007 Junta de Andalucía
#
#Autor/es (Author/s):
#
#- Gumersindo Coronel Pérez <gcoronel@emergya.info>
#
#Este fichero es parte de Detección de Hardware de Guadalinex 2005 
#
#Detección de Hardware de Guadalinex 2005  es software libre. Puede redistribuirlo y/o modificarlo 
#bajo los términos de la Licencia Pública General de GNU según es 
#publicada por la Free Software Foundation, bien de la versión 2 de dicha
#Licencia o bien (según su elección) de cualquier versión posterior. 
#
#Detección de Hardware de Guadalinex 2005  se distribuye con la esperanza de que sea útil, 
#pero SIN NINGUNA GARANTÍA, incluso sin la garantía MERCANTIL 
#implícita o sin garantizar la CONVENIENCIA PARA UN PROPÓSITO 
#PARTICULAR. Véase la Licencia Pública General de GNU para más detalles. 
#
#Debería haber recibido una copia de la Licencia Pública General 
#junto con Detección de Hardware de Guadalinex 2005 . Si no ha sido así, escriba a la Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.
#
#-------------------------------------------------------------------------
#
#This file is part of Detección de Hardware de Guadalinex 2005 .
#
#Detección de Hardware de Guadalinex 2005  is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 2 of the License, or
#at your option) any later version.
#
#Detección de Hardware de Guadalinex 2005  is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with Foobar; if not, write to the Free Software
#Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

import os
import gconf

from volume import Actor as VolumeActor

class Actor (VolumeActor):

    __required__ = {
            'volume.is_disc': True,
            'block.is_volume': True
            }


    def on_added(self):
        self.logger.debug("storage.on_add actived")
        client = gconf.client_get_default()
        is_automount = client.get_bool("/desktop/gnome/volume_manager/automount_media")
        self.logger.debug("gconf.key automount_media value is: %s", is_automount)
        if is_automount:
            self.logger.debug("gconf.key automount_media True")
            os.system("pmount %s %s" % (self.properties['block.device'], 
                      self.properties['linux.fstab.mountpoint'].split('/')[-1]))

            super(VolumeActor, self).on_added()


    def on_removed(self):
        self.logger.debug("storage.on_removed actived")
        os.system("pumount %s" % self.properties['block.device'])

        super(VolumeActor, self).on_removed()
