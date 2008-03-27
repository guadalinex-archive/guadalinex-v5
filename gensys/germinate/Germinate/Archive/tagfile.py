# -*- coding: UTF-8 -*-
"""Fetch package lists from a Debian-format archive as apt tag files."""

# Copyright (c) 2004, 2005, 2006, 2007 Canonical Ltd.
#
# Germinate is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# Germinate is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Germinate; see the file COPYING.  If not, write to the Free
# Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301, USA.

import os
import urllib2
import cStringIO
import gzip
import tempfile
import shutil

class TagFile:
    def __init__(self, mirror, source_mirror=None):
        self.mirror = mirror
        if source_mirror is not None:
            self.source_mirror = source_mirror
        else:
            self.source_mirror = mirror

    def open_tag_file(self, mirror, dirname, filename,
                      dist, component, ftppath):
        """Download an apt tag file if needed, then open it."""
        fullname = os.path.join(dirname, filename)
        if os.path.exists(fullname):
            return open(fullname, "r")

        print "Downloading", filename, "file ..."
        url = mirror + "dists/" + dist + "/" + component + "/" + ftppath
        url_f = urllib2.urlopen(url)
        url_data = cStringIO.StringIO(url_f.read())
        url_f.close()
        # apt_pkg is weird and won't accept GzipFile
        print "Decompressing", filename, "file ..."
        gzip_f = gzip.GzipFile(fileobj=url_data)
        f = open(fullname, "w")
        for line in gzip_f:
            print >>f, line,
        f.close()
        gzip_f.close()
        url_data.close()

        return open(fullname, "r")

    def feed(self, g, dists, components, arch, cleanup=False):
        if cleanup:
            dirname = tempfile.mkdtemp(prefix="germinate-")
        else:
            dirname = '.'

        for dist in dists:
            for component in components:
                filename = "%s_%s_Packages" % (dist, component)
                g.parsePackages(
                    self.open_tag_file(
                        self.mirror, dirname, filename, dist, component,
                        "binary-" + arch + "/Packages.gz"),
                    "deb")

                filename = "%s_%s_Sources" % (dist, component)
                g.parseSources(
                    self.open_tag_file(
                        self.source_mirror, dirname, filename, dist, component,
                        "source/Sources.gz"))

                instpackages = ""
                try:
                    filename = "%s_%s_InstallerPackages" % (dist, component)
                    instpackages = self.open_tag_file(
                        self.mirror, dirname, filename, dist, component,
                        "debian-installer/binary-" + arch + "/Packages.gz")
                except IOError:
                    # can live without these
                    print "Missing installer Packages file for", component, \
                          "(ignoring)"
                else:
                    g.parsePackages(instpackages, "udeb")

        if cleanup:
            shutil.rmtree(dirname)
