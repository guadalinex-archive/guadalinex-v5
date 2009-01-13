#!/bin/bash

VERSION="V5"
DIALOG="/usr/bin/dialog"

# download options file
#wget http://gensys/$GNICONF -O /etc/gni-options
test -f /etc/gni/index && rm /etc/gni/index
wget http://www.guadalinex.org/distro/$VERSION/perfiles/index -O /etc/gni/index

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
	exit 0
fi

if [ $? -eq 69 ]
then
        $DIALOG --aspect 15 --cr-wrap --title "Error en la instalación" --trim \
        --msgbox "Hubo algún problema durante la instalación de paquetes,
		 revise el fichero '/var/log/gni.log' para obtener más
		 información." 0 0
	rm $0
	exit 1
fi
