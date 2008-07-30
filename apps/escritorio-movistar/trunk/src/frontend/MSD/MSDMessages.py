#!/usr/bin/python
# -*- coding: utf-8 -*-
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

MSG_ERROR_CARDMANAGER_NOT_AVAILABLE = _(u"No se ha podido establecer comunicación con la Tarjeta o Módem USB Internet Móvil.")


#Card manager
MSG_STATUS_SEARCHING_CARD = ""
MSG_STATUS_INITIALIZING_CARD = _(u"Inicializando tarjeta")
MSG_STATUS_CARD_OFF = _(u"Tarjeta  apagada")
MSG_STATUS_WAINTING_PIN = _(u"Comprobando código PIN")
MSG_STATUS_WAINTING_PUK = _(u"Comprobando código PUK")
MSG_STATUS_CARD_ATTACHING = _(u"Buscando red y registrando")
MSG_STATUS_CARD_READY = _(u"Tarjeta preparada")
MSG_STATUS_CARD_NOT_DETECTED = _(u"Tarjeta no detectada")
MSG_STATUS_CARD_DETECTED = _(u"Tarjeta  detectada")

#dialog de Seguridad sim SIM

MSG_SIM_SEC_INSERT_PIN = _(u"Indroduzca el PIN de la tarjeta")
MSG_SIM_SEC_DEACTIVATE_PIN = _(u"El código PIN de la tarjeta SIM se encuentra activado.\nPara desactivarlo introduce su valor en la casilla\ny pulsa Aceptar.")
MSG_SIM_SEC_ACTIVATE_PIN = _(u"El código PIN de la tarjeta SIM se encuentra desactivado.\nPara activarlo introduce su valor en la casilla\ny pulsa Aceptar.")
MSG_SIM_SEC_WRONG_PIN = _(u"El código PIN introducido no es correcto.")
MSG_SIM_SEC_WRONG_PIN_FORMAT = _(u"El código PIN introducido debe tener entre 4 y 8 cifras.\nPor favor, inténtalo de nuevo.")
MSG_SIM_SEC_PIN_VERIFICATION_FAILED = _(u"El PIN y la verificación del PIN no coinciden.")
MSG_SIM_SEC_PUK_FAILED = _(u"El código PUK introducido no es correcto.")
MSG_AT_COMMAND_FAILED = _(u"No se ha podido establecer contacto con la Tarjeta o Módem USB Internet Móvil.")

MSG_PUK_INSERTION_CANCELED = _(u"Has cancelado el proceso de comprobación del código PUK y tu Tarjeta o Módem USB Internet Móvil se apagará\n\nPara volver a iniciar el proceso enciéndela desde el menú de tarjeta a través de la opción 'Activar tarjeta'.")
MSG_PUK_INSERTION_CANCELED_TITLE = _(u"Comprobación cancelada")

MSG_PIN_INSERTION_CANCELED = _(u"Has cancelado el proceso de comprobación del código PIN y tu Tarjeta o Módem USB Internet Móvil se apagará.\n\nPara volver a iniciar el proceso enciéndela desde el menú de tarjeta a través de la opción 'Activar tarjeta'.")


MSG_PIN_INSERTION_CANCELED_TITLE = _(u"Comprobación cancelada")

MSG_PIN_CHANGE_FAILED = _(u"Ha fallado el proceso de cambio de PIN de la tarjeta SIM.\nA continuación se apagará la Tarjeta o Módem USB Internet Móvil. La siguiente vez que se encienda se pedirá el código PUK de la misma.")
MSG_PIN_CHANGE_FAILED_TITLE = _(u"Error en el cambio de PIN")


#Estados del pin
MSG_SIM_PIN_ACTIVE = _(u"Activado")
MSG_SIM_PIN_NO_ACTIVE = _(u"Desactivado")

MSG_PIN_ACTIVE_INFO = _(u"El código PIN de la tarjeta SIM se encuentra activado.\nPara desactivarlo introduce su valor en la casilla y pulsa el botón Aceptar.")

MSG_PIN_INACTIVE_INFO = _(u"El código PIN de la tarjeta SIM se encuentra desactivado.\nPara activarlo introduce su valor en la casilla y pulsa el botón Aceptar.")

#buscar operadora
MSG_CARRIER_IS_LONG_OPERATION_TITLE = _(u"Selección manual de operadora")
MSG_CARRIER_IS_LONG_OPERATION = _(u"A continuación se van a buscar las operadoras disponibles. Esta operación puede tardar varios minutos. Para comenzar la búsqueda pulsa en Aceptar.")

MSG_PIN_MANAGE_FAILED = _(u"Ha fallado el proceso de activación/desactivación  del código PIN de la tarjeta SIM.\nA continuación se apagará la Tarjeta o Módem USB Internet Móvil. La siguiente vez que se encienda se pedirá el código PUK de la misma.")

MSG_PIN_MANAGE_FAILED_TITLE = _(u"Error en la activación/desactivación  del  PIN")

MSG_SEARCHING_CARRIERS = _(u"Por favor, espera mientras se buscan las operadoras disponibles.\nEste proceso puede tardar varios minutos.")
MSG_SEARCHING_CARRIERS_TITLE = _(u"Buscando operadoras")

MSG_CARRIER_SEARCH_ERROR = _(u"Se ha producido un error mientras se buscaban las operadoras disponibles.")
MSG_CARRIER_ATTACH_ERROR = _(u"Se ha producido un error mientras se configuraba la operadora seleccionada.")

MSG_ATTACHING_CARRIER = _(u"Por favor, espera mientras se configura la operadora\nseleccionada.")
MSG_ATTACHING_CARRIER_TITLE = _(u"Configurando operadora")

MSG_RESET_CONSUM_WARNING = _(u"Al reiniciar se eliminará la información del consumo acumulado.")

#Bookmarks

MSG_DELETE_BOOKMARK_TITLE = _(u"<b>Eliminar acceso directo</b>")
MSG_DELETE_BOOKMARK = _(u"¿Deseas eliminar el acceso directo '%s'?")

MSG_ADD_BOOKMARK_TITLE = _(u"Nuevo acceso directo")
MSG_EDIT_BOOKMARK_TITLE = _(u"Modificar acceso directo")

MSG_INVALID_BOOKMARK_NAME_TITLE = _(u"<b>Nombre de acceso directo no válido</b>")
MSG_INVALID_BOOKMARK_NAME = _(u"El nombre del acceso directo no puede contener el carácter '/'.")

MSG_NO_BOOKMARK_NAME_TITLE = _(u"<b>Nombre de acceso directo</b>")
MSG_NO_BOOKMARK_NAME = _(u"Introduce un nombre para el acceso directo.")

MSG_BOOKMARK_NAME_EXISTS_TITLE = _(u"<b>Ya existe un acceso directo con el nombre '%s'</b>")
MSG_BOOKMARK_NAME_EXISTS = _(u"Por favor, elige otro nombre.")

MSG_BOOKMARK_NO_FILE_SELECTED_TITLE = _(u"<b>No has seleccionado un archivo</b>")
MSG_BOOKMARK_NO_FILE_SELECTED = _(u"Por favor, selecciona un archivo para la creación del acceso directo.")

MSG_BOOKMARK_NO_URL_SELECTED_TITLE = _(u"<b>No has introducido la URL</b>")
MSG_BOOKMARK_NO_URL_SELECTED = _(u"Por favor, introduce una URL para la creación del acceso directo.")

#Connections

MSG_DELETE_CONNECTION_TITLE = _(u"<b>Eliminar conexión</b>")
MSG_DELETE_CONNECTION = _(u"¿Deseas eliminar la conexión '%s'?")

MSG_ADD_CONNECTION_TITLE = _(u"Nueva conexión")
MSG_EDIT_CONNECTION_TITLE = _(u"Modificar conexión")

MSG_CONNECTION_NAME_EXISTS_TITLE = _(u"<b>Ya existe una conexión con el nombre '%s'</b>")
MSG_CONNECTION_NAME_EXISTS = _(u"Por favor, elige otro nombre")

MSG_NO_CONNECTION_NAME_TITLE = _(u"<b>Nombre de conexión</b>")
MSG_NO_CONNECTION_NAME = _(u"Introduce un nombre para la conexión.")

MSG_NO_CONNECTION_USER_TITLE = _(u"<b>Introduce un usuario</b>")
MSG_NO_CONNECTION_USER = _(u"No has introducido el nombre de usuario para la conexión.")

MSG_NO_CONNECTION_PASSWORD_TITLE = _(u"<b>Introduce la contraseña</b>")
MSG_NO_CONNECTION_PASSWORD = _(u"No has introducido la contraseña de la conexión.")

MSG_NO_CONNECTION_PROFILE_TITLE = _(u"<b>El campo perfil personalizado está vacío</b>")
MSG_NO_CONNECTION_PROFILE = _(u"Por favor, indica el perfil personalizado que deseas utilizar.")

MSG_NO_CONNECTION_DNS_TITLE = _(u"<b>La información de DNS no está completa</b>")
MSG_NO_CONNECTION_DNS = _(u"Por favor, rellena debidamente los campos correspondientes.")

MSG_NO_CONNECTION_PROXY_IP_TITLE = _(u"<b>La dirección del Proxy no es válida</b>")
MSG_NO_CONNECTION_PROXY_IP = _(u"Introduce una dirección válida.")

MSG_NO_CONNECTION_PROXY_PORT_TITLE = _(u"<b>El puerto del Proxy no es válido</b>")
MSG_NO_CONNECTION_PROXY_PORT = _(u"Los valores permitidos son enteros positivos.")

#Connections Manager

MSG_CONN_MANAGER_NO_PPP_MANAGER_TITLE = _(u"<b>No es posible la comunicación con el PPP Manager</b>")
MSG_CONN_MANAGER_NO_PPP_MANAGER = _(u"La conexión a través del PPP Manager no ha podido ser realizada. Lo que significa que por el momento no es posible la conexión a Internet mediante esta aplicación.")

MSG_CONN_MANAGER_ASK_PASSWORD = _(u"<b>Introduce la contraseña de usuario para la conexión '%s'</b>")

MSG_CONN_MANAGER_NO_CARDMANAGER_TITLE = _(u"<b>No es posible conectar</b>")
MSG_CONN_MANAGER_NO_CARDMANAGER = _(u"La Tarjeta o Módem USB Internet Móvil no está preparada para conectar. Por favor insértala o enciéndela si se encuentra apagada y espera a que se registre en la red.")

MSG_CONN_MANAGER_APP_CLOSE_TITLE = _(u"<b>Confirmar fin de la aplicación</b>")
MSG_CONN_MANAGER_APP_CLOSE = _(u"El Escritorio movistar está conectado. Si sales de la aplicación la conexión se cerrará. ¿Deseas continuar?.")

MSG_CONN_MANAGER_NO_DEVICE_TITLE = _(u"<b>No hay ningún dispositivo seleccionado</b>")
MSG_CONN_MANAGER_NO_DEVICE = _(u"No se puede realizar la conexión solicitada debido a que no hay ningún dispositivo. Por favor, selecciona un dispositivo en la ventana de configuración.")

MSG_CONN_MANAGER_CONNECTION_ERROR_TITLE = _(u"<b>Error en la conexión</b>")
MSG_CONN_MANAGER_CONNECTION_ERROR = _(u"No ha sido posible realizar la conexión. Por favor, inténtalo de nuevo.")

MSG_CONN_MANAGER_ACTIVE_CONN_DETECT_TITLE = _(u"<b>Se ha detectado otra conexión activa</b>")

MSG_CONN_MANAGER_OPEN_SERVICE_TITLE = _(u"<b>Abrir servicio '%s'</b>")
MSG_CONN_MANAGER_OPEN_SERVICE = _(u"¿Deseas conectarte con la conexión asociada al servicio? (%s)")
MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN = _(u"En este momento estás conectado con '%s'. ¿Deseas abrir el servicio cambiando a su conexión asociada '%s' o prefieres usar la conexión actual?\nSi cambias la conexión puedes perder información en otros servicios y accesos directos abiertos.")
MSG_CONN_MANAGER_OPEN_SERVICE_WITH_ACTIVE_CONN_DEFAULT = _(u"En este momento estás conectado con '%s'. ¿Deseas abrir el servicio cambiando a su conexión por defecto '%s' o prefieres usar la conexión actual?\nSi cambias la conexión puedes perder información en otros servicios y accesos directos abiertos.")


MSG_CONN_MANAGER_OPEN_BOOKMARK_TITLE = _(u"<b>Abrir acceso directo '%s'</b>")
MSG_CONN_MANAGER_OPEN_BOOKMARK = _(u"¿Deseas conectarte con la conexión asociada al acceso directo? (%s)")
MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN = _(u"En este momento estás conectado con '%s'. ¿Deseas abrir el acceso directo cambiando a su conexión asociada '%s' o prefieres usar la conexión actual?\nSi cambias la conexión puedes perder información en otros servicios y accesos directos abiertos.")
MSG_CONN_MANAGER_OPEN_BOOKMARK_WITH_ACTIVE_CONN_DEFAULT = _(u"En este momento estás conectado con '%s'. ¿Deseas abrir el acceso directo cambiando a su conexión por defecto '%s' o prefieres usar la conexión actual?\nSi cambias la conexión puedes perder información en otros servicios y accesos directos abiertos.")

MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_TITLE = _(u"<b>Conectar con la conexión predeterminada</b>")
MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_WITH_ACTIVE_CONN = _(u"En este momento estás conectado con '%s'. ¿Deseas abrir la conexión '%s' o prefieres usar la conexión actual?\nSi cambias la conexión puedes perder información en otros servicios y accesos directos abiertos.")

MSG_CONN_MANAGER_CONNECT_TO_DEFAULT_CONN = _(u"¿Deseas conectarte con la conexión predeterminada para el Escritorio movistar (%s)?")

MSG_INVALID_PHONE_TITLE = _(u"<b>El número de teléfono no es válido</b>")
MSG_INVALID_PHONE =  _(u"Los valores permitidos son caracteres numéricos.")
