#!/bin/bash

GNICONF="gni-options"

# download options file
wget http://gensys/$GNICONF -O /etc/gni-options
if [ $? -ne 0 ]
then
	$DIALOG --aspect 15 --cr-wrap --title "Error de conexión" --trim \
        --msgbox "No se pudo descargar la configuración del instalador.
		 La instalación no ha sido completada, se intentará completar
		 la próxima vez que se inicie el sistema." 0 0
	exit 0
fi

# launch menu app
/usr/sbin/gni

# remove myself (/sbin/setup.sh) if apt-get install was launched
if [ $? -eq 0 ]
then
	rm $0
fi
