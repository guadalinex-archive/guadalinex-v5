# -*- coding: utf-8 -*-
# Configuración útil para master.cfg

# SVN 
svn = "file:///var/svn/gv5"
apps_dir = "apps"
metapkgs_dir = "metapkgs"
trunk_dir = "trunk"
tags_dir = "tags"
gcs_dir = "gcs"

apps=[
	"casper-guada", 
	"gcs", 
	"gfxboot-theme-guada", 
	"guadalinex-keyring", 
	"lig",
	"usplash-theme-guadalinex"
]

metapkgs = [
	"guadalinex-artwork", 
	"guadalinex-desktop", 
	"guadalinex-desktop-conf", 
	"guadalinex-minimal",
	"guadalinex-minimal-conf", 
	"guadalinex-standard",
	"guadalinex-standard-conf",
	"meta-guadalinex-v5"
]

# Build
app_timer = 5
metapkg_timer = 5
genera_iso_timer = 60
halt_on_lintian_error = 0
halt_on_unittest_error = 0

# Upload
upload_dir = "/var/mirror/pkgs"
