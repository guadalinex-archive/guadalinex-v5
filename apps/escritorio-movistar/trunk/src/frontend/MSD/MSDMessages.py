#!/usr/bin/python
# -*- coding: iso-8859-15 -*-
#
# Authors : Roberto Majadas <roberto.majadas@openshine.com>
#           Oier Blasco <oierblasco@gmail.com>
#           Alvaro Pe�a <alvaro.pena@openshine.com>
#
# Copyright (c) 2003-2007, Telefonica M�viles Espa�a S.A.U.
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

MSG_ERROR_CARDMANAGER_NOT_AVAILABLE = u"No se ha podido establecer comunicaci�n con la Tarjeta o M�dem USB Internet M�vil."


#Card manager
MSG_STATUS_SEARCHING_CARD = u""
MSG_STATUS_INITIALIZING_CARD = u"Inicializando tarjeta"
MSG_STATUS_CARD_OFF = u"Tarjeta  apagada"
MSG_STATUS_WAINTING_PIN = u"Comprobando c�digo PIN"
MSG_STATUS_WAINTING_PUK = u"Comprobando c�digo PUK"
MSG_STATUS_CARD_ATTACHING = u"Buscando red y registrando"
MSG_STATUS_CARD_READY = u"Tarjeta preparada"
MSG_STATUS_CARD_NOT_DETECTED = u"Tarjeta no detectada"
MSG_STATUS_CARD_DETECTED = u"Tarjeta  detectada"

#dialog de Seguridad sim SIM

MSG_SIM_SEC_INSERT_PIN = u"Indroduzca el PIN de la tarjeta"
MSG_SIM_SEC_DEACTIVATE_PIN = u"El c�digo PIN de la tarjeta SIM se encuentra activado.\nPara desactivarlo introduce su valor en la casilla\ny pulsa Aceptar."
MSG_SIM_SEC_ACTIVATE_PIN = u"El c�digo PIN de la tarjeta SIM se encuentra desactivado.\nPara activarlo introduce su valor en la casilla\ny pulsa Aceptar."
MSG_SIM_SEC_WRONG_PIN = u"El c�digo PIN introducido no es correcto."
MSG_SIM_SEC_WRONG_PIN_FORMAT = u"El c�digo PIN introducido debe tener entre 4 y 8 cifras.\nPor favor, int�ntalo de nuevo."
MSG_SIM_SEC_PIN_VERIFICATION_FAILED = u"El PIN y la verificaci�n del PIN no coinciden."
MSG_SIM_SEC_PUK_FAILED = u"El c�digo PUK introducido no es correcto."
MSG_AT_COMMAND_FAILED = u"No se ha podido establecer contacto con la Tarjeta o M�dem USB Internet M�vil."

MSG_PUK_INSERTION_CANCELED = u"Has cancelado el proceso de comprobaci�n del c�digo PUK y tu Tarjeta o M�dem USB Internet M�vil se apagar�\n\nPara volver a iniciar el proceso enci�ndela desde el men� de tarjeta a trav�s de la opci�n 'Activar tarjeta'."
MSG_PUK_INSERTION_CANCELED_TITLE = u"Comprobaci�n cancelada"

MSG_PIN_INSERTION_CANCELED = u"Has cancelado el proceso de comprobaci�n del c�digo PIN y tu Tarjeta o M�dem USB Internet M�vil se apagar�.\n\nPara volver a iniciar el proceso enci�ndela desde el men� de tarjeta a trav�s de la opci�n 'Activar tarjeta'."


MSG_PIN_INSERTION_CANCELED_TITLE = u"Comprobaci�n cancelada"

MSG_PIN_CHANGE_FAILED = u"Ha fallado el proceso de cambio de PIN de la tarjeta SIM.\nA continuaci�n se apagar� la Tarjeta o M�dem USB Internet M�vil. La siguiente vez que se encienda se pedir� el c�digo PUK de la misma."
MSG_PIN_CHANGE_FAILED_TITLE = u"Error en el cambio de PIN"


#Estados del pin
MSG_SIM_PIN_ACTIVE = u"Activado"
MSG_SIM_PIN_NO_ACTIVE = u"Desactivado"

MSG_PIN_ACTIVE_INFO = u"El c�digo PIN de la tarjeta SIM se encuentra activado.\nPara desactivarlo introduce su valor en la casilla y pulsa el bot�n Aceptar."

MSG_PIN_INACTIVE_INFO = u"El c�digo PIN de la tarjeta SIM se encuentra desactivado.\nPara activarlo introduce su valor en la casilla y pulsa el bot�n Aceptar."

#buscar operadora
MSG_CARRIER_IS_LONG_OPERATION_TITLE = u"Selecci�n manual de operadora"
MSG_CARRIER_IS_LONG_OPERATION = u"A continuaci�n se van a buscar las operadoras disponibles. Esta operaci�n puede tardar varios minutos. Para comenzar la b�squeda pulsa en Aceptar."

MSG_PIN_MANAGE_FAILED = u"Ha fallado el proceso de activaci�n/desactivaci�n  del c�digo PIN de la tarjeta SIM.\nA continuaci�n se apagar� la Tarjeta o M�dem USB Internet M�vil. La siguiente vez que se encienda se pedir� el c�digo PUK de la misma."

MSG_PIN_MANAGE_FAILED_TITLE = u"Error en la activaci�n/desactivaci�n  del  PIN"

MSG_SEARCHING_CARRIERS = u"Por favor, espera mientras se buscan las operadoras disponibles.\nEste proceso puede tardar varios minutos."
MSG_SEARCHING_CARRIERS_TITLE = u"Buscando operadoras"

MSG_CARRIER_SEARCH_ERROR = u"Se ha producido un error mientras se buscaban las operadoras disponibles."
MSG_CARRIER_ATTACH_ERROR = u"Se ha producido un error mientras se configuraba la operadora seleccionada."

MSG_ATTACHING_CARRIER = u"Por favor, espera mientras se configura la operadora\nseleccionada."
MSG_ATTACHING_CARRIER_TITLE = u"Configurando operadora"

MSG_RESET_CONSUM_WARNING = u"Al reiniciar se eliminar� la informaci�n del consumo acumulado."

#Bookmarks

MSG_DELETE_BOOKMARK_TITLE = u"<b>Eliminar acceso directo</b>"
MSG_DELETE_BOOKMARK = u"�Deseas eliminar el acceso directo '%s'?"

MSG_ADD_BOOKMARK_TITLE = u"Nuevo acceso directo"
MSG_EDIT_BOOKMARK_TITLE = u"Modificar acceso directo"

MSG_NO_BOOKMARK_NAME_TITLE = u"<b>Nombre de acceso directo</b>"
MSG_NO_BOOKMARK_NAME = u"Introduce un nombre para el acceso directo."

MSG_BOOKMARK_NAME_EXISTS_TITLE = u"<b>Ya existe un acceso directo con el nombre '%s'</b>"
MSG_BOOKMARK_NAME_EXISTS = u"Por favor, elige otro nombre."

MSG_BOOKMARK_NO_FILE_SELECTED_TITLE = u"<b>No has seleccionado un archivo</b>"
MSG_BOOKMARK_NO_FILE_SELECTED = u"Por favor, selecciona un archivo para la creaci�n del acceso directo."

MSG_BOOKMARK_NO_URL_SELECTED_TITLE = u"<b>No has introducido la URL</b>"
MSG_BOOKMARK_NO_URL_SELECTED = u"Por favor, introduce una URL para la creaci�n del acceso directo."

#Connections

MSG_DELETE_CONNECTION_TITLE = u"<b>Eliminar conexi�n</b>"
MSG_DELETE_CONNECTION = u"�Deseas eliminar la conexi�n '%s'?"

MSG_ADD_CONNECTION_TITLE = u"Nueva conexi�n"
MSG_EDIT_CONNECTION_TITLE = u"Modificar conexi�n"

MSG_CONNECTION_NAME_EXISTS_TITLE = u"<b>Ya existe una conexi�n con el nombre '%s'</b>"
MSG_CONNECTION_NAME_EXISTS = u"Por favor, elige otro nombre"

MSG_NO_CONNECTION_NAME_TITLE = u"<b>Nombre de conexi�n</b>"
MSG_NO_CONNECTION_NAME = u"Introduce un nombre para la conexi�n."

MSG_NO_CONNECTION_USER_TITLE = u"<b>Introduce un usuario</b>"
MSG_NO_CONNECTION_USER = u"No has introducido el nombre de usuario para la conexi�n."

MSG_NO_CONNECTION_PASSWORD_TITLE = u"<b>Introduce la contrase�a</b>"
MSG_NO_CONNECTION_PASSWORD = u"No has introducido la contrase�a de la conexi�n."

MSG_NO_CONNECTION_PROFILE_TITLE = u"<b>El campo perfil personalizado est� vac�o</b>"
MSG_NO_CONNECTION_PROFILE = u"Por favor, indica el perfil personalizado que deseas utilizar."

MSG_NO_CONNECTION_DNS_TITLE = u"<b>La informaci�n de DNS no est� completa</b>"
MSG_NO_CONNECTION_DNS = u"Por favor, rellena debidamente los campos correspondientes."

MSG_NO_CONNECTION_PROXY_IP_TITLE = u"<b>La direcci�n del Proxy no es v�lida</b>"
MSG_NO_CONNECTION_PROXY_IP = u"Introduce una direcci�n v�lida."

MSG_NO_CONNECTION_PROXY_PORT_TITLE = u"<b>El puerto del Proxy no es v�lido</b>"
MSG_NO_CONNECTION_PROXY_PORT = u"Los valores permitidos son enteros positivos."

#Connections Manager

MSG_CONN_MANAGER_NO_PPP_MANAGER_TITLE = u"<b>No es posible la comunicaci�n con el PPP Manager</b>"
MSG_CONN_MANAGER_NO_PPP_MANAGER = u"La conexi�n a trav�s del PPP Manager no ha podido ser realizada. Lo que significa que por el momento no es posible la conexi�n a Internet mediante esta aplicaci�n."

MSG_CONN_MANAGER_ASK_PASSWORD = u"<b>Introduce la contrase�a de usuario para la conexi�n '%s'</b>"

MSG_CONN_MANAGER_NO_CARDMANAGER_TITLE = u"<b>No es posible conectar</b>"
MSG_CONN_MANAGER_NO_CARDMANAGER = u"La Tarjeta o M�dem USB Internet M�vil no est� preparada para conectar. Por favor ins�rtala o enci�ndela si se encuentra apagada y espera a que se registre en la red."

MSG_CONN_MANAGER_APP_CLOSE_TITLE = u"<b>Confirmar fin de la aplicaci�n</b>"
MSG_CONN_MANAGER_APP_CLOSE = u"El Escritorio movistar est� conectado. Si sales de la aplicaci�n la conexi�n se cerrar�. �Deseas continuar?."

MSG_CONN_MANAGER_NO_DEVICE_TITLE = u"<b>No hay ning�n dispositivo seleccionado</b>"
MSG_CONN_MANAGER_NO_DEVICE = u"No se puede realizar la conexi�n solicitada debido a que no hay ning�n dispositivo. Por favor, selecciona un dispositivo en la ventana de configuraci�n."

MSG_CONN_MANAGER_CONNECTION_ERROR_TITLE = u"<b>Error en la conexi�n</b>"
MSG_CONN_MANAGER_CONNECTION_ERROR = u"No ha sido posible realizar la conexi�n. Por favor, int�ntalo de nuevo."

MSG_CONN_MANAGER_ACTIVE_CONN_DETECT_TITLE = u"<b>Se ha detectado otra conexi�n activa</b>"

MSG_CONN_MANAGER_OPEN_SERVICE_TITLE = u"<b>Abrir servicio '%s'</b>"
MSG_CONN_MANAGER_OPEN_SERVICE = u"�Deseas conectarte con la conexi�n asociada al servicio? (%s)"
MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN = u"En este momento est�s conectado con '%s'. �Deseas abrir el servicio cambiando a su conexi�n asociada '%s' o prefieres usar la conexi�n actual?\nSi cambias la conexi�n puedes perder informaci�n en otros servicios y accesos directos abiertos."
MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN_DEFAULT = u"En este momento est�s conectado con '%s'. �Deseas abrir el servicio cambiando a su conexi�n por defecto '%s' o prefieres usar la conexi�n actual?\nSi cambias la conexi�n puedes perder informaci�n en otros servicios y accesos directos abiertos."


MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE = u"<b>Abrir acceso directo '%s'</b>"
MSG_CONN_MANAGER_OPEN_BOOKMARK = u"�Deseas conectarte con la conexi�n asociada al acceso directo? (%s)"
MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN = u"En este momento est�s conectado con '%s'. �Deseas abrir el acceso directo cambiando a su conexi�n asociada '%s' o prefieres usar la conexi�n actual?\nSi cambias la conexi�n puedes perder informaci�n en otros servicios y accesos directos abiertos."
MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN_DEFAULT = u"En este momento est�s conectado con '%s'. �Deseas abrir el acceso directo cambiando a su conexi�n por defecto '%s' o prefieres usar la conexi�n actual?\nSi cambias la conexi�n puedes perder informaci�n en otros servicios y accesos directos abiertos."

MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_TITLE = u"<b>Conectar con la conexi�n predeterminada</b>"
MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_WITH_ACTIVE_CONN = u"En este momento est�s conectado con '%s'. �Deseas abrir la conexi�n '%s' o prefieres usar la conexi�n actual?\nSi cambias la conexi�n puedes perder informaci�n en otros servicios y accesos directos abiertos."

MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_CONN = u"�Deseas conectarte con la conexi�n predeterminada para el Escritorio movistar (%s)?"

MSG_INVALID_PHONE_TITLE = u"<b>El n�mero de tel�fono no es v�lido</b>"
MSG_INVALID_PHONE =  u"Los valores permitidos son caracteres num�ricos."
