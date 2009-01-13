#!/bin/bash

VERSION="V5"
DIALOG="/usr/bin/dialog"
INDEXURL="http://www.guadalinex.org/distro/$VERSION/perfiles/index"

# show index download url confirmation
$DIALOG --aspect 15 --cr-wrap --nocancel --title "Confirmación de la URL" \
--backtitle "Instalación de Guadalinex Mínima" \
--trim --inputbox "Por favor, confirme que esta es la URL de indices que quiere \
usar o modifiquela manualmente:" 0 100 "$URL" 2> /tmp/gni-url

# reset index download url from the inputbox
INDEXURL=$(cat /tmp/gni-url)

# download index file
test -f /etc/gni/index && rm /etc/gni/index
wget $INDEXURL -O /etc/gni/index

# show any error if there was any
if [ $? -ne 0 ]
then
	$DIALOG --aspect 15 --cr-wrap --title "Error de conexión" --trim \
	--backtitle "Instalación de Guadalinex Mínima" \
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
	--backtitle "Instalación de Guadalinex Mínima" \
        --msgbox "Hubo algún problema durante la instalación de paquetes,
		 revise el fichero '/var/log/gni.log' para obtener más
		 información." 0 0
	rm $0
	exit 1
fi
