#!/bin/bash

# Remove for develpers (if use links)
[ -L  /usr/bin/gs_new ] && rm /usr/bin/gs_new
[ -L  /usr/bin/gs_build ] && rm /usr/bin/gs_build
[ -L  /usr/share/doc/guadalinex-supplement-generator ] && rm /usr/share/doc/guadalinex-supplement-generator
rm /usr/share/guadalinex-supplement-generator
