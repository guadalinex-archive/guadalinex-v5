#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Peña <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica Móviles España S.A.U.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.

# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.

# You should have received a copy of the GNU General Public
# License along with this library; if not, write to the Free
# Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#


import os

#from pkg_resources import require
#require("sqlobject>=0.7.0")
from sqlobject import *

def get_user_addressbook():
    db_path = os.path.join(os.environ["HOME"],".movistar_desktop","contacts") 
    db_path = os.path.abspath(db_path)
    return MDAddressBook(db_path)
    
class MDContact(SQLObject):
    class sqlmeta:
        table = "contacts"
    
    name  = StringCol()
    phone = StringCol()
    email = StringCol()
    copia_agenda_id = StringCol()
    modification_stringdate = StringCol()

class MDAddressBook:
    """
    Esta clase representa una agenda
    """
    def __init__(self,db_path):
        self.__init_db(db_path)
        
        connection_string = 'sqlite:' + db_path
        connection = connectionForURI(connection_string)
        sqlhub.processConnection = connection
        
    def __init_db(self,db_path):
         conf_dir = os.path.dirname(db_path)
         if not os.path.exists(conf_dir):
             os.makedirs(conf_dir)
            
         connection_string = 'sqlite:' + db_path
         connection = connectionForURI(connection_string)
         sqlhub.processConnection = connection

         # creo tablas
         MDContact.createTable(ifNotExists=True)

    def get_all_with_mail(self):
        return MDContact.select(MDContact.q.email != "")
    
    def get_all_contacts(self):
        return MDContact.select()

    def search_people(self):
        pass
