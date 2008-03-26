#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import os.path

from  config import config


class Part(object):
    """ <abstrat>
    """
    def get_postinst_content(self):
        raise NotImplementedError


    def get_prerm_content(self):
        raise NotImplementedError



class AlternativesPart(Part):
    pass


class DivertPart(Part):
    """ Generate dpkg-divert commands from conffiles_skel dir.
    """
    def __init__(self):
        self.postinst_content = ''
        self.diverts = []
        self.removes = []


    def get_postinst_content(self):
        self.__add_divert_content(self.__add_divert)
        return ''.join(self.diverts)


    def get_prerm_content(self):
        self.__add_divert_content(self.__rm_divert)
        return ''.join(self.removes)


    def __add_divert_content(self, divert_method):
        orig_stuff_len = len(config['source_path'] + '/')
        dest_stuff_len = len(config['source_path'] + '/gcs/conffiles_skel/')


        def set_divert(arg, dirname, filesnames):
            extension = config['config_extension']
            for fname in filesnames:
                base_path = dirname + os.sep + fname
                orig_path = base_path[orig_stuff_len:]
                dest_path = '/' + base_path[dest_stuff_len:]

                abs_path = dirname + '/' + fname
                if (not '/.svn' in orig_path) and\
                        os.path.isfile(abs_path) and\
                        abs_path.endswith(extension):
                    divert_method(dest_path)

                desktop_extension = 'desktop' + extension
                if fname.endswith(desktop_extension):
                    if divert_method == self.__add_divert:
                        real_conf_path = dest_path[: -len(extension)]
                        command = "rm %s\n" % real_conf_path
                        command += "cp %s %s\n\n" % (dest_path, real_conf_path)
                        self.diverts.append(command)

                    elif divert_method == self.__rm_divert:
                        pass

        os.path.walk(config['source_path'] + '/gcs/conffiles_skel', 
                set_divert, None)
                    
        return "divert command"



    def __add_divert(self, dest_path):
        extlen = len(config['config_extension'])
        real_conf_path = dest_path[: -extlen]

        pkg_name = config['info']['name']
        divert_command = '[ "%s" != "$(dpkg-divert --truename %s)" ] && rm -f %s && dpkg-divert --rename --remove %s\n' % (real_conf_path, real_conf_path, real_conf_path, real_conf_path)

        divert_command += "dpkg-divert --package %s --rename " % pkg_name
        divert_command += "--quiet --add %s\n" % real_conf_path

        divert_command += "ln -fs %s %s\n\n" % (dest_path, real_conf_path)

        self.diverts.append(divert_command)



    def __rm_divert(self, dest_path):
        extlen = len(config['config_extension'])
        real_conf_path = dest_path[: -extlen]

        pkg_name = config['info']['name']
        command = "rm -f %s\n" % real_conf_path
        command += "dpkg-divert --package %s " % pkg_name
        command += "--rename --quiet --remove %s\n" % real_conf_path

        # rm -f /etc/ejemplo.conf
        # dpkg-divert --package dummy-conf --rename --quiet --remove /etc/ejemplo.conf
        self.removes.append(command)


class ScriptsPart(Part):

    def __init__(self, scripts_dir_path):
        self.scripts_dir_path = scripts_dir_path
        self.scripts = []


    def get_scripts_content(self):

        def insert_script(arg, dirname, filenames):
            for fname in filenames:
                abs_path = dirname + '/' + fname
                if os.path.isfile(abs_path) and \
                        (not '/.svn' in abs_path):
                    self.__insert_script(abs_path)

        os.path.walk(self.scripts_dir_path, insert_script, None)

        return '\n'.join(self.scripts)

    
    def __insert_script(self, script_path):
        content = open(script_path).read() + '\n'
        self.scripts.append(content)
