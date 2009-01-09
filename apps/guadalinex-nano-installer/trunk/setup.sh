#!/bin/bash

GNICONF="gni-options"

# download options file
wget http://gensys/$GNICONF -O /etc/gni-options

# launch menu app
/usr/sbin/gni

# remove myself (/sbin/setup.sh) if apt-get install was launched
if [ $? -eq 0 ]
then
	rm $0
fi
