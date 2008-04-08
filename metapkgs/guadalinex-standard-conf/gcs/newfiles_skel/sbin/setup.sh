#!/bin/sh
# setup.sh - Ajustes para el sistema tras (re) instalación

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin/:/usr/local/sbin

frontend=noninteractive

debconf-communicate -fnoninteractive xserver-xorg > /dev/null <<EOF
set xserver-xorg/autodetect_keyboard true
fset xserver-xorg/autodetect_keyboard seen true
set xserver-xorg/config/inputdevice/keyboard/layout es
fset xserver-xorg/config/inputdevice/keyboard/layout seen true
EOF

LANG=es_ES.UTF-8 dpkg-reconfigure -fnoninteractive --no-reload xserver-xorg

# Sólo hay que hacer la configuración una vez
rm /sbin/setup.sh
