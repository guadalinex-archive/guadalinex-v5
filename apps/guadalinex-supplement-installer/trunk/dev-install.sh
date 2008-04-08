#!/bin/bash

# Install for developers (using links)
DIR=$(pwd)
ln -s $DIR/guadalinex-app-install /usr/bin
ln -s $DIR/gsd  /usr/share/
ln -s $DIR/glsuppletory.py  /usr/share/hermes/actors/
ln -s $DIR/gsd.png /usr/share/pixmaps/
ln -s $DIR/guadalinex-app-install.desktop /usr/share/applications/

