#!/usr/bin/python

import os
from subprocess import Popen, PIPE

GUADAFOX_DIR="guadafox-0.1"

changes_in_files = [("ubufox@ubuntu.com", "guadafox@guadalinex.org", False),
                    ("ubuntuHelpMenuOverlay.xul", "guadalinexHelpMenuOverlay.xul", False),
                    ("ubuntuAddonsOverlay", "guadalinexAddonsOverlay", False),
                    ("<em:version>0.5~rc1</em:version>", "<em:version>0.1</em:version>", True),
                    ("<em:creator>Canonical Ltd.</em:creator>", "<em:creator>Junta de Andalucia</em:creator>", True),
                    ("<em:contributor>Alexander Sack &lt;asac@ubuntu.com&gt;</em:contributor>",
                     "<em:contributor>Alexander Sack &lt;asac@ubuntu.com&gt;</em:contributor>\n    <em:contributor>Roberto Majadas &lt;roberto.majadas@openshine.com&gt;</em:contributor>",
                     True),
                    ("ubuntulogo32.png", "guadalinexlogo32.png", False),
                    ("app.releaseNotesURL=http://www.ubuntu.com/download/releasenotes/804", 
                     "app.releaseNotesURL=http://www.guadalinex.org", False),
                    ("file:///usr/share/ubuntu-artwork/home/locales-ubuntu/", "file:///usr/share/guadalinex-artwork/home/locales-guadalinex/", False),
                    ("file:///usr/share/ubuntu-artwork/", "file:///usr/share/guadalinex-artwork/", False),
                    ("getubuntuextension", "getguadalinexextension", False),
                    ("ubufox", "guadafox", False),
                    ("Ubuntu", "Guadalinex", False)]

if os.path.exists(GUADAFOX_DIR) :
    os.system("rm -fr %s" % GUADAFOX_DIR)

p = Popen(["find -type f"], shell=True, stdin=PIPE, stdout=PIPE, close_fds=True)
file_list = p.stdout.readlines()
ubufox_files = []

for x in file_list:
    if x != ".\n":
        ubufox_files.append(x.strip("\n"))

os.system("mkdir -p %s" % GUADAFOX_DIR)

for f in ubufox_files :
    fd = open(f)
    in_file = fd.readlines()
    out_file = ""
    for line in in_file :
        for patt in changes_in_files:
            if patt[0] in line :
                line = line.replace(patt[0], patt[1])
                if patt[2] == True:
                    break
        out_file = out_file + line
    
    gdir = os.path.dirname(f)
    gfile = os.path.basename(f)

    os.system("mkdir -p ./%s/%s" % (GUADAFOX_DIR, gdir))

    if "ubuntu" in gfile :
        gfile = gfile.replace("ubuntu", "guadalinex")

    if "ubufox" in gfile :
        gfile = gfile.replace("ubufox", "guadafox")

    gfd = open("./%s/%s/%s" % (GUADAFOX_DIR, gdir, gfile), "w")
    gfd.write(out_file)
    gfd.close()
    print "write ./%s/%s/%s" % (GUADAFOX_DIR, gdir, gfile)

os.system("tar czf %s.tar.gz %s" % (GUADAFOX_DIR,GUADAFOX_DIR))
os.system("rm -fr %s" % (GUADAFOX_DIR))
os.system("mv  %s.tar.gz ../" % (GUADAFOX_DIR))
