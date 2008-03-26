#!/usr/bin/python
# -*- coding: utf-8 -*-
""" Parse /etc/gcs.conf for configuration options
"""

import sys

import syck

try:
    config = syck.load(open('/etc/gcs.conf').read())

    # Add default options
    config['source_path'] = './'
    config['info'] = {}
    
except:
    print "Can't read /etc/gcs.conf file."
    sys.exit(1)

