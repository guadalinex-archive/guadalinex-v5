#!/bin/sh
PAQUETES_DEB=$(cat conf/override.lobo.deb | awk '{print $1}')
for i in $PAQUETES_DEB; do 
	[ -z $i ] && continue
	DEBS=$(grep ${i}_ dists/hardy/*/*/Packages | grep Filename | awk '{print $2}')
	for o in $DEBS; do
		cp $o tmp/
	done
	reprepro -VVV remove lobo ${i}
done 
reprepro -VVV includedeb lobo tmp/*
rm tmp/*

i=''
o=''

PAQUETES_UDEB=$(cat conf/override.lobo.udeb | awk '{print $1}')
for i in $PAQUETES_UDEB; do
	[ -z $i ] && continue
	UDEBS=$(grep ${i}_ dists/hardy/*/debian-installer/*/Packages | grep Filename | awk '{print $2}')
	for o in $UDEBS; do
		cp $o tmp/
	done
	reprepro -VVV remove lobo ${i}
done
reprepro -VVV includeudeb lobo tmp/*
rm tmp/*
