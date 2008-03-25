#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import shutil
import os.path
import re
import datetime
import email.Utils

from config import config
from generators.part import DivertPart
from generators.part import ScriptsPart 


class FileGenerator(object):

    def __init__(self):
        self.template_content = ''

    
    def activate(self):
        raise NotImplementedError


    def set_template_content(self, template_name):
        """ Set template content from  template_name (using config dictionary)

        @param template_name: Key of the config dictionary for template.
        @type template_name: string
        """
        try:
            template_file = open(config[template_name])
            self.template_content = template_file.read()
            template_file.close()
        except KeyError:
            print "Don't find template '%s'" % template_name
        except:
            print "Can't create template content. Template: %s" % template_name


    
    def _write_file(self, path):
        real_file = open(config['source_path'] + '/' + path, 'w')
        real_file.write(self.template_content)
        real_file.close()


class ControlGenerator(FileGenerator):
    """ Generate debian/control file from gcs/info file.
    """
    def activate(self):
        """ Generate debian/control file

        Steps:

        1) Obtain control template
        2) Set template properties (using tags) from gcs/info file.
        3) Write debian/control file
        """
        self.set_template_content('control_template')

        self.__set_name()
        self.__set_author()
        self.__set_predepends()
        self.__set_depends()
        self.__set_conflicts()
        self.__set_section()
        self.__set_priority()
        self.__set_descriptions()

        self._write_file('debian/control')


    def __set_name(self):
        newcontent = self.template_content.replace('<NAME>', config['info']['name'])
        self.template_content = newcontent


    def __set_author(self):
        author = config['info']['author']
        newcontent = self.template_content.replace('<MANTAINER>', author)
        self.template_content = newcontent


    def __set_predepends(self):
        predepends = self.__parse_deps('/gcs/predepends')
        newcontent = self.template_content.replace('<PREDEPENDS>', predepends)
        self.template_content = newcontent


    def __set_depends(self):
        depends = self.__parse_deps('/gcs/depends')
        newcontent = self.template_content.replace('<DEPENDS>', depends)
        self.template_content = newcontent


    def __set_conflicts(self):
        conflicts = self.__parse_deps('/gcs/conflicts')
        newcontent = self.template_content.replace('<CONFLICTS>', conflicts)
        self.template_content = newcontent


    def __set_section(self):
        section = config['info']['section']
        newcontent = self.template_content.replace('<SECTION>', section)
        self.template_content = newcontent

    def __set_priority(self):
        priority = config['info']['priority']
        newcontent = self.template_content.replace('<PRIORITY>', priority)
        self.template_content = newcontent

    def __set_descriptions(self):
        shortdesc = config['info']['shortdesc']
        longdesc = config['info']['longdesc']
        newcontent = self.template_content.replace('<SHORTDESC>', shortdesc)
        newcontent = newcontent.replace('<LONGDESC>', longdesc)
        self.template_content = newcontent

    def __parse_deps(self, file):
        try:
            depends_list = open(config['source_path'] + file).readlines()
        except IOError:
            #print "No existe el fichero gcs/predepends"
            return ''
    
        new_depends = []
        for depend in depends_list:
            depend = depend.strip()
            if not depend or depend.startswith('#'):
                continue
            name_and_version = depend.split()

            depend_string = name_and_version[0]
            if len(name_and_version) == 2:
                depend_string += " (%s)" %(name_and_version[1])

            new_depends.append(depend_string)

        depends = ', '.join(new_depends)
        return depends



class RulesGenerator(FileGenerator):
    """ Generates debian/rules.

    Generates debian/rules file based on "newfiles_skel" 
    and "conffiles_skel" directories, and "newfiles" file.
    """

    def __init__(self):
        self.dhinstall_list = []
        self.dirs = []
        FileGenerator.__init__(self)


    def activate(self):
        self.set_template_content('rules_template')

        self.__process_newfiles()
        self.__process_skel('newfiles_skel')
        self.__process_skel('conffiles_skel')
        self.__write_rules_file()


    def __process_newfiles(self):
        """ Process "newfiles" file looking for files to install.
        """
        if os.path.isfile(config['source_path'] + '/gcs/newfiles'):
            newfiles_lines = open(config['source_path'] + '/gcs/newfiles').readlines()
    
            for line in newfiles_lines:
                line = line.strip()
                line_tuple = line.split()
                if (len(line_tuple) != 2) or line.startswith('#'):
                    continue
    
                self.__add_dhinstall(*line_tuple)


    def __process_skel(self, skel_name):
        """ Process skel_name directory recursively.

        Process skel_name directory recursively 
        looking for files to install.
        """ 
        orig_stuff_len = len(config['source_path'] + '/')
        dest_stuff_len = len(config['source_path'] + '/gcs/' + \
                skel_name + '/')

        def set_dhinstall(arg, dirname, file_names):
            if not '/.svn' in dirname:
                dir_to_add = dirname[dest_stuff_len - 1:]
                if dir_to_add:
                    self.dirs.append(dirname[dest_stuff_len - 1:])    

            for fname in file_names:
                base_path = dirname + os.sep + fname
                orig_path = base_path[orig_stuff_len:]
                dest_path = base_path[dest_stuff_len:]

                if (not '/.svn' in orig_path) and\
                        os.path.isfile(orig_path):
                    dest_path = os.path.dirname(dest_path)
                    self.__add_dhinstall(orig_path, dest_path)

        os.path.walk(config['source_path'] + '/gcs/' + skel_name, 
                set_dhinstall, None)


    def __write_rules_file(self):
        dhinstall_content = '\n'.join(self.dhinstall_list)
        newcontent = self.template_content.replace('<DHINSTALL_SLOT>', 
                dhinstall_content)
        self.template_content = newcontent

        self._write_file('debian/rules')
        os.chmod(config['source_path'] + '/' + 'debian/rules', 0755)

        # write debian/dirs file
        dirs_file = open(config['source_path'] + '/debian/dirs', 'w')
        dirs_file.write('\n'.join(self.dirs))
        dirs_file.close()


    def __add_dhinstall(self, orig_path, dest_path):
        if not dest_path:
            return
        #dest_path = os.path.dirname(dest_path)
        command = ''
	    # If we aren't working with config files or we are working with them but has the appropiate
	    # extension fill the command
        if not ('gcs/conffiles_skel/' in orig_path) or orig_path.endswith(config['config_extension']):
            exclude_arg = ''
            if os.path.isdir(orig_path + '/.svn'):
                exclude_arg = '--exclude=.svn'
            command = '\tdh_install %s "%s" "%s"' % (exclude_arg, orig_path, dest_path)

        if command:
            self.dhinstall_list.append(command)



class ChangelogGenerator(FileGenerator):

    def __init__(self):
        try:
            self.actual_content = open(config['source_path'] + \
                '/gcs/changelog').read()
            self.changelog_exists = True
        except:
            self.actual_content = ''
            self.changelog_exists = False

        FileGenerator.__init__(self)


    def activate(self):
        debchangelog_path = config['source_path'] + '/debian/changelog'
        if self.__is_new_version():
            self.set_template_content('changelog_template')

            self.__set_basic_info()
            self.__set_changes()

            self.template_content += '\n\n' + self.actual_content
            self._write_file('gcs/changelog')
            self._write_file('debian/changelog')
        elif self.changelog_exists and (not os.path.exists(debchangelog_path)):
            orig_changelog_path = config['source_path'] + '/gcs/changelog'
            shutil.copy(orig_changelog_path, debchangelog_path)


    def __set_basic_info(self):
        info = config['info']
        newcontent = self.template_content.replace('<NAME>',
                info['name'])
        newcontent = newcontent.replace('<VERSION>', 
                str(info['version']))
        newcontent = newcontent.replace('<AUTHOR>', 
                info['author'])
        newcontent = newcontent.replace('<DATE>',
                email.Utils.formatdate()) 

        self.template_content = newcontent


    def __set_changes(self):
        changes_str = ''
        if not config['info']['changes']: 
            config['info']['changes'] = ['No changes']
        for change in config['info']['changes']:
            changes_str += '  * %s\n' % change

        newcontent = self.template_content.replace('<CHANGES>',
                changes_str)
        self.template_content = newcontent


    def __is_new_version(self):
        """ Check if there is a new version of package.
        """
        content = None
        try:
            content = open('gcs/changelog').readlines()
        except:
            pass
        
        if not content: return True

        old_version = re.findall('\((.*)\)', content[0])[0]
        new_version = str(config['info']['version'])

        if old_version == new_version:
            return False
        else:
            return True

class PrePostGenerator(FileGenerator):
    """<abstract>
    """

    def __init__(self):
        FileGenerator.__init__(self)
        self.scripts = []

        # Params for derivated classes.
        self.template_name = ''
        self.file_path = ''
        self.divert_content = ''
        self.scripts_path = ''


    def activate(self):
        self.set_template_content(self.template_name)
        initial_content = self.template_content
        initial_content = initial_content.replace('<DIVERT_SLOT>', '')
        initial_content = initial_content.replace('<SCRIPTS_SLOT>', '')
        
        self._set_divert()
        self._set_install_scripts()

        if initial_content != self.template_content:
            self._write_file(self.file_path)
        else:
            try:
                os.remove(config['source_path'] + '/' + self.file_path)
            except:
                pass


    def _set_divert(self):
        newcontent = self.template_content.replace('<DIVERT_SLOT>', 
               self.divert_content)
        self.template_content = newcontent


    def _set_install_scripts(self):
        scripts_part = ScriptsPart(config['source_path'] + \
                '/' + self.scripts_path)

        scripts_content = scripts_part.get_scripts_content()
        newcontent = self.template_content.replace('<SCRIPTS_SLOT>',
                scripts_content)
        self.template_content = newcontent



class PreInstGenerator(PrePostGenerator):

    def __init__(self):
        PrePostGenerator.__init__(self)

        self.template_name = 'preinst_template'
        self.file_path = 'debian/preinst'
        self.scripts_path = 'gcs/install_scripts/pre/'



class PostInstGenerator(PrePostGenerator):

    def __init__(self):
        PrePostGenerator.__init__(self)

        self.template_name = 'postinst_template'
        self.file_path = 'debian/postinst'
        self.divert_content = DivertPart().get_postinst_content()
        self.scripts_path = 'gcs/install_scripts/pos/'



class PreRmGenerator(PrePostGenerator):

    def __init__(self):
        PrePostGenerator.__init__(self)

        self.template_name = 'prerm_template'
        self.file_path = 'debian/prerm'
        self.divert_content = DivertPart().get_prerm_content()
        self.scripts_path = 'gcs/remove_scripts/pre/'



class PostRmGenerator(PrePostGenerator):

    def __init__(self):
        PrePostGenerator.__init__(self)

        self.template_name = 'postrm_template'
        self.file_path = 'debian/postrm'
        self.scripts_path = 'gcs/remove_scripts/pos/'



class CompatGenerator(FileGenerator):

    def activate(self):
        self.set_template_content('compat_template')
        self._write_file('debian/compat')



class CopyrightGenerator(FileGenerator):

    def activate(self):
        self.set_template_content('copyright_template')
        self._write_file('debian/copyright')
