#!/usr/bin/python
# -*- coding: utf-8 -*-

'''
AUTHOR: Daniel Garcia <dgarcia@emergya.es>

Este script descarga todo el contenido dentro de un div especificado jutno con
todos los enlaces dentro del dominio especificado.

ejemplo:

<div id="content">
    ...
    <a href="/guadapedia/index.php/Guia_v5/Gratitud" title ="Guia
        v5/Gratitud"> Gratitud</a>
    ...
</div>

descargaria todo lo englobado dentro de este div, y llamaria a esta funcion de
manera recursiva para cada enlace que no cumpla alguno de los filtros
especificados en URLS_TO_IGNORE 

DIV_ID = 'content'

'''

import sys
import os
import urllib2
import re
import shutil

from BeautifulSoup import BeautifulSoup
from grabparser import grabParser

URL_BASE = 'http://www.guadalinex.org/'
INIT_RESOURCE = 'guadapedia/index.php/Guía_de_usuario_para_Guadalinex_V5'
DIV_ID = 'content'
DEST_DIR = 'manual'
CSS_DIR = 'css-skel'

URLS = []
DEPTH = 0
LIMIT_DEPTH = 3

# This head was getted from a mediawiki page
HTML_HEAD = '''
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="es" lang="es" dir="ltr">
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8"/>

    <title>%(title)s</title>

    <link / REL="shortcut icon" HREF="imgs/favicon.ico">
    <link / REL="stylesheet" TYPE="text/css" MEDIA="screen,projection" HREF="css/main.css">
    <link / REL="stylesheet" TYPE="text/css" MEDIA="print" HREF="css/commonPrint.css">
    <script SRC="css/script.js" TYPE="text/javascript"></script>
    <script SRC="css/wikibits.js" TYPE="text/javascript"></script>
    
  </head>
  <body class="ns-0">
  <div ID="globalWrapper">

'''

HTML_ENDING = '''
</div>
</body>
</html>
'''

# That urls will be ignored and not follow
URLS_TO_IGNORE = ['^.*&action=edit$',\
                '^(http|https)://.*$',\
                '^javascript:.*$',\
                '.*#.*$']

URLS_TO_IGNORE = map(re.compile, URLS_TO_IGNORE)
IMAGE_WIKI_EXP = re.compile('^Imagen:.*')

def copy_css_skel():
    try:
        shutil.copytree(CSS_DIR,DEST_DIR+"/css")
    except Exception, inst:
        print inst

def save_file(filename, content):
    try:
        output = open(os.path.join(DEST_DIR, filename), 'w')
        output.write(str(content))
        output.close()
    except Exception, inst:
        print inst

def save(list_of_urls, dest='css.orig'):
    try:
        os.mkdir(os.path.join(DEST_DIR, dest))
    except Exception, inst:
        pass
        #print inst

    for i in list_of_urls:
        name = i.split('/')[-1]
        try:
            file = open(os.path.join(DEST_DIR, dest, name), 'w')
        except:
            continue
        if i[0] == '/':
            i = i[1:]
        try:
            content = urllib2.urlopen(URL_BASE + i).read()
            file.write(content)
        except Exception, inst:
            print inst
        file.close()
        
def save_css(html):
    '''
    Look for all references to a css file in the html and try to save
    '''
    css_exp = re.compile('"(/.*\.css)"')
    
    css_s = css_exp.findall(html)

    save(css_s, 'css.orig')
    return css_s

def exist(filename):
    '''
    True if exists filename or filename.html in filesystem
    '''
    return os.path.exists(filename) or os.path.exists(filename + '.html')

def fix_image(filename):
    '''
    Fix the image name for mediawiki
    Imagen:image.png -> image.png, True
    piripipiripi -> piripipiripi, False
    '''


    isimg = False
    if IMAGE_WIKI_EXP.match(filename):
        filename = filename.replace('Imagen:', '')
        isimg = True
    return filename, isimg

def get_wiki_image(url):
    '''
    Get the image inside the div fullImage
    '''

    try:
        first_page = urllib2.urlopen(URL_BASE + url)
    except Exception, inst:
        print 'imagen: ', URL_BASE + urllib2.unquote(url), 'no existe'
        return False

    data = first_page.read()
    try:
        soup = BeautifulSoup(data)
    except Exception, inst:
        return True

    content = soup.find('div', {'class':'fullImage'}, True)

    parser = grabParser()
    parser.parse(str(content))
    if len(parser.imgs) > 0:
        save(parser.imgs, 'imgs')

    return True


def grab(secondary_url):
    global DEPTH
    global LIMIT_DEPTH

    if (secondary_url, DEPTH) in URLS:
        return True

    URLS.append((secondary_url, DEPTH))

    filename = urllib2.unquote(secondary_url.split('/')[-1])
    filename, isimg = fix_image(filename)

    if exist(filename):
        return True

    if isimg:
        get_wiki_image(secondary_url)
        return True

    if DEPTH > LIMIT_DEPTH:
        return True

    print '[%d]' % DEPTH, '--' * DEPTH, 'parsing...\t', urllib2.unquote(secondary_url)
    DEPTH += 1

    try:
        first_page = urllib2.urlopen(URL_BASE + secondary_url)
    except Exception, inst:
        print URL_BASE + urllib2.unquote(secondary_url), 'no existe'
        return False

    data = first_page.read()
    try:
        soup = BeautifulSoup(data)
    except Exception, inst:
        save_file(filename, data)
        return True

    css_tuple = save_css(str(soup))
    
    content = soup.find('div', {'id':DIV_ID}, True)

    parser = grabParser()
    parser.parse(str(content))

    first_page.close()

    save(parser.imgs, 'imgs')


    content = str(content)
    for i in parser.hyperlinks:
        cont = False

        newi = i
        if i[0] == '/':
            newi = i[1:]

        # Ignoring urls
        for expr in URLS_TO_IGNORE:
            if expr.match(i):
                cont = True
                break
                
        if cont:
            continue

        # Recursive call
        grab(newi)

        # fixing links
        new_url = newi.split('/')[-1]
        new_url, isimg = fix_image(new_url)

        links_exp = re.compile('<a href="'+i+'"')
        if isimg:
            content = links_exp.sub('<a href="imgs/' + new_url + '"', content) 
        else:
            content = links_exp.sub('<a href="' + new_url + '.html"', content) 

    # fixing imgs
    for i in parser.imgs:
        new_img = i.split('/')[-1]
        images_exp = re.compile('<img src="'+i+'"')
        content = images_exp.sub('<img src="imgs/' + new_img + '"', content)

    # Adding all css file in all html
    css_string = ''
    for i in css_tuple:
        css_string += '\n@import "css/' + i.split('/')[-1] + '";'

    content = (HTML_HEAD % {'title': filename, 'css': css_string}) + content + HTML_ENDING
    save_file(filename + '.html', content)

    DEPTH -= 1

    return True
               

if __name__ == '__main__':
    print 'Init...'
    result = grab(INIT_RESOURCE)
    print result
    print 'Copying css...'
    copy_css_skel()
