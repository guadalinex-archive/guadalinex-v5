#!/bin/bash

# Este script es el que ejecuta la interfaz gráfica de usuario de guadalinex-asistente-usb.
# Basado en el script de nanomax-installer hecho por MarioDebian
#
#

LOG=/var/log/guadalinex-asistente-usb
rm $LOG
exec 2> >(while read a; do echo "$(date):" "$a"; done >>$LOG) 

DEVICE=$1
PERSISTENTE=$2


if [ "$DEVICE" = "" ] ; then
  zenity --error --text="Error en los parámetros:\nDispositivo: '$DEVICE'\nPersistente: '$PERSISTENTE'"
  exit 1
fi

if [ ! -d /cdrom ]; then
  zenity --error --text="No se esta ejecutando desde el CD live de Guadalinex... saliendo"
  exit 1
fi

# leemos el tamaño
SIZE=$(LC_ALL=C /sbin/fdisk -l $DEVICE| awk '/^Disk*.*bytes/ {print $3}')

echo " * Tamaño del dispositivo $DEVICE detectado $SIZE Mb"


desmonta(){
    for dev in $(grep $DEVICE /proc/mounts | awk '{print $2}'); do
      umount -l $dev
    done
}

gconf_active_automount(){
	if [ "$1" == 0 ];then
	    sudo -u usuario gconftool-2 --type bool -s /apps/nautilus/preferences/media_automount false
	    sudo -u usuario gconftool-2 --type bool -s /apps/nautilus/preferences/media_automount_open false
        elif [ "$1" == 1 ];then
	    sudo -u usuario gconftool-2 --type bool -s /apps/nautilus/preferences/media_automount true
	    sudo -u usuario gconftool-2 --type bool -s /apps/nautilus/preferences/media_automount_open true
	fi
}

# desmontamos particiones
sync
desmonta


gconf_active_automount 0

zenity --question --text="Si continúa se formateará el contenido de $DEVICE y se perderán todos los archivos y particiones \n¿Desea seguir?"
if [ $? != 0 ]; then
  echo " * Cancelado !!!!!"
  exit 1
fi

copiar_guadalinex() {
  (
  echo " * Copiando archivos de Guadalinex... (tarda un rato)"
#  cp -a /cdrom/* /mnt/guadav5/
#  los llaveros generados con cp dan problemas al arrancar
  rsync -Pazv /cdrom/ /mnt/guadav5/
  mv /mnt/guadav5/isolinux/* /mnt/guadav5/
  rm -rf /mnt/guadav5/isolinux/
  cp /usr/share/guadalinex-asistente-usb/syslinux.cfg /mnt/guadav5/
  )| zenity --progress --auto-close --pulsate --text="Copiando Guadalinex...(tarda un rato)"
}

prepare_partitions(){
        sfdisk -q -uM $1 <<EOF 
,750,b
,,L
EOF
        fdisk $1 <<EOF 
a
1
w
EOF
}



# borrar MBR
dd if=/dev/zero of=$DEVICE bs=512 count=1 >/dev/null 2>&1

mkdir -p /mnt/guadav5

## particionado
prepare_partitions $DEVICE
sleep 1
mkfs.vfat -F 32 -n "guadav5" "${1}1"
mkfs.ext2 -b 4096 -L "home-rw" "${1}2"
mount -t vfat -o noatime,rw ${DEVICE}1 /mnt/guadav5
copiar_guadalinex
echo " * Sincronizando... (puede tardar un rato)"
sync
syslinux ${DEVICE}1
umount /mnt/guadav5
install-mbr -e1 ${DEVICE}

gconf_active_automount 1

zenity --info --text="Ya esta lista su Guadalinex USB live!, inicie su equipo desde el dispositivo USB."

echo " * Borrando directorios temporales"
rm -rf /mnt/guadav5

exit 0

