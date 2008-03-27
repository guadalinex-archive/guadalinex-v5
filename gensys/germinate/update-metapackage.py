#!/usr/bin/env python
# -*- coding: UTF-8 -*-

# Copyright (c) 2004, 2005, 2006, 2007, 2008 Canonical Ltd.
# Copyright (c) 2006 Gustavo Franco
#
# This file is part of Germinate.
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

# TODO:
# - Exclude essential packages from dependencies

import sys
import re
import os
import getopt
import logging
import ConfigParser

try:
    set # introduced in 2.4
except NameError:
    import sets
    set = sets.Set

import apt_pkg

from Germinate import Germinator
import Germinate.Archive
import Germinate.seeds
import Germinate.version

def usage(f):
    print >>f, """Usage: update-metapackage.py [options]

Options:

  -h, --help            Print this help message and exit.
  --version             Output version information and exit.
  --bzr                 Fetch seeds using bzr. Requires bzr to be installed.
"""

bzr = False

try:
    opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "version", "bzr"])
except getopt.GetoptError:
    usage(sys.stderr)
    sys.exit(2)

for option, value in opts:
    if option in ("-h", "--help"):
        usage(sys.stdout)
        sys.exit()
    elif option == "--version":
        print "%s %s" % (os.path.basename(sys.argv[0]),
                         Germinate.version.VERSION)
        sys.exit()
    elif option == "--bzr":
        bzr = True

if not os.path.exists('debian/control'):
    raise RuntimeError('must be run from the top level of a source package')
this_source = None
control = open('debian/control')
for line in control:
    if line.startswith('Source:'):
        this_source = line[7:].strip()
        break
    elif line == '':
        break
if this_source is None:
    raise RuntimeError('cannot find Source: in debian/control')
if not this_source.endswith('-meta'):
    raise RuntimeError('source package name must be *-meta')
metapackage = this_source[:-5]

print "[info] Initialising %s-* package lists update..." % metapackage
    
config = ConfigParser.SafeConfigParser()
config_file = open('update.cfg')
config.readfp(config_file)
config_file.close()

if len(args) > 0:
    dist = args[0]
else:
    dist = config.get('DEFAULT', 'dist')
        
seeds = config.get(dist, 'seeds').split()
try:
    output_seeds = config.get(dist, 'output_seeds').split()
except ConfigParser.NoOptionError:
    output_seeds = list(seeds)
architectures = config.get(dist, 'architectures').split()
try:
    archive_base_default = config.get(dist, 'archive_base/default')
except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
    archive_base_default = None

archive_base = {}
for arch in architectures:
    try:
        archive_base[arch] = config.get(dist, 'archive_base/%s' % arch)
    except (ConfigParser.NoSectionError, ConfigParser.NoOptionError):
        if archive_base_default is not None:
            archive_base[arch] = archive_base_default
        else:
            raise RuntimeError('no archive_base configured for %s' % arch)

if bzr and config.has_option("%s/bzr" % dist, 'seed_base'):
    seed_base = config.get("%s/bzr" % dist, 'seed_base')
else:
    seed_base = config.get(dist, 'seed_base')
seed_base = re.split(r'[, ]+', seed_base)
if bzr and config.has_option("%s/bzr" % dist, 'seed_dist'):
    seed_dist = config.get("%s/bzr" % dist, 'seed_dist')
elif config.has_option(dist, 'seed_dist'):
    seed_dist = config.get(dist, 'seed_dist')
else:
    seed_dist = dist
seed_entry = re.compile(' *\* *(?P<package>\S+) *(\[(?P<arches>[^]]*)\])? *(#.*)?')
components = config.get(dist, 'components').split()

def seed_packages(germinator_list, seed, seed_text):
    if config.has_option(dist, "seed_map/%s" % seed):
        mapped_seeds = config.get(dist, "seed_map/%s" % seed).split()
    else:
        mapped_seeds = []
        task_seeds_re = re.compile('^Task-Seeds:\s*(.*)', re.I)
        for line in seed_text:
            task_seeds_match = task_seeds_re.match(line)
            if task_seeds_match is not None:
                mapped_seeds = task_seeds_match.group(1).split()
                break
        if seed not in mapped_seeds:
            mapped_seeds.append(seed)
    packages = []
    for mapped_seed in mapped_seeds:
        packages.extend(germinator_list[mapped_seed])
    return packages

def metapackage_name(seed, seed_text):
    if config.has_option(dist, "metapackage_map/%s" % seed):
        return config.get(dist, "metapackage_map/%s" % seed)
    else:
        task_meta_re = re.compile('^Task-Metapackage:\s*(.*)', re.I)
        for line in seed_text:
            task_meta_match = task_meta_re.match(line)
            if task_meta_match is not None:
                return task_meta_match.group(1)
        return "%s-%s" % (metapackage, seed)

debootstrap_version_file = 'debootstrap-version'

def get_debootstrap_version():
    version = os.popen("dpkg-query -W --showformat '${Version}' debootstrap").read()
    if not version:
        raise RuntimeError('debootstrap does not appear to be installed')

    return version

def debootstrap_packages(arch):
    debootstrap = os.popen('debootstrap --arch %s --print-debs %s debootstrap-dir %s' % (arch,dist,archive_base[arch]))
    packages = debootstrap.read().split()
    if debootstrap.close():
        raise RuntimeError('Unable to retrieve package list from debootstrap')
    
    
    # sometimes debootstrap gives empty packages / multiple separators
    packages = filter(None, packages)
    
    packages.sort()

    return packages

def check_debootstrap_version():
    if os.path.exists(debootstrap_version_file):
        old_debootstrap_version = open(debootstrap_version_file).read().strip()
        debootstrap_version = get_debootstrap_version()
        failed = os.system("dpkg --compare-versions '%s' ge '%s'" % (debootstrap_version,
                                                                     old_debootstrap_version))
        if failed:
            raise RuntimeError('Installed debootstrap is older than in the previous version! (%s < %s)' % (
                debootstrap_version,
                old_debootstrap_version
                ))

def update_debootstrap_version():
    open(debootstrap_version_file, 'w').write(get_debootstrap_version() + '\n')

logger = logging.getLogger()
logger.setLevel(logging.DEBUG)
handler = logging.StreamHandler(sys.stdout)
handler.setFormatter(logging.Formatter('%(levelname)s%(message)s'))
logger.addHandler(handler)

check_debootstrap_version()

additions = {}
removals = {}
moves = {}
apt_pkg.InitConfig()
for architecture in architectures:
    print "[%s] Downloading available package lists..." % architecture
    apt_pkg.Config.Set("APT::Architecture", architecture)
    germinator = Germinator()
    Germinate.Archive.TagFile(archive_base[architecture], archive_base_default).feed(
        germinator, [dist], components, architecture, cleanup=True)
    debootstrap_base = set(debootstrap_packages(architecture))

    print "[%s] Loading seed lists..." % architecture
    seed_names, seed_inherit, seed_branches = germinator.parseStructure(
        seed_base, seed_dist, bzr)
    needed_seeds = []
    for seed_name in seeds:
        for inherit in seed_inherit[seed_name]:
            if inherit not in needed_seeds:
                needed_seeds.append(inherit)
        if seed_name not in needed_seeds:
            needed_seeds.append(seed_name)
    seed_texts = {}
    for seed_name in needed_seeds:
        seed_fd = Germinate.seeds.open_seed(seed_base, seed_branches,
                                            seed_name, bzr)
        seed_texts[seed_name] = seed_fd.readlines()
        seed_fd.close()
        germinator.plantSeed(seed_texts[seed_name], architecture, seed_name,
                             list(seed_inherit[seed_name]))

    print "[%s] Merging seeds with available package lists..." % architecture
    for seed_name in output_seeds:
        meta_name = metapackage_name(seed_name, seed_texts[seed_name])

        output_filename = '%s-%s' % (seed_name,architecture)
        old_list = None
        if os.path.exists(output_filename):
            old_list = set(map(str.strip,open(output_filename).readlines()))
            os.rename(output_filename, output_filename + '.old')

        # work on the depends
        new_list = []
        for package in seed_packages(germinator.seed, seed_name,
                                     seed_texts[seed_name]):
            if package == meta_name:
                print "%s/%s: Skipping package %s (metapackage)" % (seed_name,architecture,package)
                continue
            if seed_name == 'minimal' and package not in debootstrap_base:
                print "%s/%s: Skipping package %s (package not in debootstrap)" % (seed_name,architecture,package)
            else:
                new_list.append(package)

        new_list.sort()
        output = open(output_filename, 'w')
        for package in new_list:
            output.write(package)
            output.write('\n')
        output.close()

        # work on the recommends
        old_recommends_list = None
        new_recommends_list = []
        for package in seed_packages(germinator.seedrecommends, seed_name,
                                     seed_texts[seed_name]):
            if package == meta_name:
                print "%s/%s: Skipping package %s (metapackage)" % (seed_name,architecture,package)
                continue
            if seed_name == 'minimal' and package not in debootstrap_base:
                print "%s/%s: Skipping package %s (package not in debootstrap)" % (seed_name,architecture,package)
            else:
                new_recommends_list.append(package)

        new_recommends_list.sort()
        output_recommends_filename = '%s-recommends-%s' % (seed_name,architecture)
        if os.path.exists(output_recommends_filename):
            old_recommends_list = set(map(str.strip,open(output_recommends_filename).readlines()))
            os.rename(output_recommends_filename, output_recommends_filename + '.old')

        output = open(output_recommends_filename, 'w')
        for package in new_recommends_list:
            output.write(package)
            output.write('\n')
        output.close()


        # Calculate deltas
        merged = {}
        recommends_merged = {}
        if old_list is not None:
            for package in new_list:
                merged.setdefault(package, 0)
                merged[package] += 1
            for package in old_list:
                merged.setdefault(package, 0)
                merged[package] -= 1
        if old_recommends_list is not None:
            for package in new_recommends_list:
                recommends_merged.setdefault(package, 0)
                recommends_merged[package] += 1
            for package in old_recommends_list:
                recommends_merged.setdefault(package, 0)
                recommends_merged[package] -= 1

        mergeditems = merged.items()
        mergeditems.sort()
        for package, value in mergeditems:
            #print package, value
            if value == 1:
                if recommends_merged.get(package, 0) == -1:
                    moves.setdefault(package,[])
                    moves[package].append(output_filename)
                    recommends_merged[package] += 1
                else:
                    additions.setdefault(package,[])
                    additions[package].append(output_filename)
            elif value == -1:
                if recommends_merged.get(package, 0) == 1:
                    moves.setdefault(package,[])
                    moves[package].append(output_recommends_filename)
                    recommends_merged[package] -= 1
                else:
                    removals.setdefault(package,[])
                    removals[package].append(output_filename)

        mergedrecitems = recommends_merged.items()
        mergedrecitems.sort()
        for package, value in mergedrecitems:
            #print package, value
            if value == 1:
                additions.setdefault(package,[])
                additions[package].append(output_recommends_filename)
            elif value == -1:
                removals.setdefault(package,[])
                removals[package].append(output_recommends_filename)


if additions or removals:
    dch_help = os.popen('dch --help')
    have_U = '-U' in dch_help.read()
    dch_help.close()
    if have_U:
        os.system("dch -iU 'Refreshed dependencies'")
    else:
        os.system("dch -i 'Refreshed dependencies'")
    changes = []
    addition_keys = additions.keys()
    addition_keys.sort()
    for package in addition_keys:
        changes.append('Added %s to %s' %
                       (package, ', '.join(additions[package])))
    removal_keys = removals.keys()
    removal_keys.sort()
    for package in removal_keys:
        changes.append('Removed %s from %s' %
                       (package, ', '.join(removals[package])))
    move_keys = moves.keys()
    move_keys.sort()
    for package in move_keys:
        # TODO: We should really list where it moved from as well, but that
        # gets wordy very quickly, and at the moment this is only
        # implemented for depends->recommends or vice versa. In future,
        # using this for moves between seeds might also be useful.
        changes.append('Moved %s to %s' %
                       (package, ', '.join(moves[package])))
    for change in changes:
        print change
        os.system("dch -a '%s'" % change)
    update_debootstrap_version()
else:
    print "No changes found"
