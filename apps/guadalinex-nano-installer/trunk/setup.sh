#!/bin/bash

GNICONF="gni-options"

# download options file
wget http://gensys/$GNICONF -O /etc/gni-options

# launch menu app
/usr/sbin/gni

# remove myself (/sbin/setup.sh)
rm $0
