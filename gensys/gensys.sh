#!/bin/bash

. $(dirname $0)/functions

# parse script arguments and act in consecuence
menu "$@"

export CDIMAGE_ROOT=/var/opt/cdimage
export DIST=$distro
export PATH="$CDIMAGE_ROOT/bin:$PATH"

update_repository

create_live_image

create_iso

