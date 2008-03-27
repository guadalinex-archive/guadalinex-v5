#!/bin/bash

# Script to build everything possible : sources and binaries for all archs

. CONF.sh

rm -rf "$TDIR"
rm -rf "$OUT"
TMP_OUT=$OUT

if [ -z "$ARCHES" ]; then
	export ARCHES='amd64 i386 powerpc'
fi

for FULLARCH in $ARCHES
do
	export ARCH="${FULLARCH%%+*}"
	if [ "$ARCH" = "$FULLARCH" ]; then
		export SUBARCH=
	else
		export SUBARCH="${FULLARCH#*+}"
	fi
	echo "Now we're going to build CD for $FULLARCH !"
	if type find-mirror >/dev/null 2>&1; then
		# TODO: nasty upcall to cdimage wrapper scripts
		export MIRROR="$(find-mirror "$ARCH")"
		if [ "$SECURITY" ]; then
			export SECURITY="$MIRROR"
		fi
	fi
	echo " ... cleaning"
	make distclean
	make ${CODENAME}_status
	echo " ... checking your mirror"
	if [ "$SKIPMIRRORCHECK" != "yes" ] ; then 
		make mirrorcheck-binary
      	if [ "$FULLARCH" = "i386" ]; then
            make mirrorcheck-source
        fi
	else
		echo "WARNING: skipping mirrorcheck"
	fi

	if [ $? -gt 0 ]; then
		echo "ERROR: Your mirror has a problem, please correct it." >&2
		exit 1
	fi
	echo " ... selecting packages to include"
	if [ -e ${MIRROR}/dists/${CODENAME}/main/disks-${ARCH}/current/. ] ; then
		disks=`du -sm ${MIRROR}/dists/${CODENAME}/main/disks-${ARCH}/current/. | \
				awk '{print $1}'`
	else
		disks=0
	fi
	if [ -f $BASEDIR/tools/boot/$CODENAME/boot-$FULLARCH.calc ]; then
	    . $BASEDIR/tools/boot/$CODENAME/boot-$FULLARCH.calc
	fi
	SIZE_ARGS=''
	for CD in 1; do
		size=`eval echo '$'"BOOT_SIZE_${CD}"`
		[ "$size" = "" ] && size=0
		[ $CD = "1" ] && size=$(($size + $disks))
        mult=`eval echo '$'"SIZE_MULT_${CD}"`
        [ "$mult" = "" ] && mult=100
        FULL_SIZE=`echo "($DEFBINSIZE - $size) * 1024 * 1024" | bc`
        echo "INFO: Reserving $size MB on CD $CD for boot files.  SIZELIMIT=$FULL_SIZE."
        if [ $mult != 100 ]; then
            echo "  INFO: Reserving "$((100-$mult))"% of the CD for extra metadata"
            FULL_SIZE=`echo "$FULL_SIZE * $mult" / 100 | bc`
            echo "  INFO: SIZELIMIT now $FULL_SIZE."
        fi
        SIZE_ARGS="$SIZE_ARGS SIZELIMIT${CD}=$FULL_SIZE"
	done
    FULL_SIZE=`echo "($DEFSRCSIZE - $size) * 1024 * 1024" | bc`
	echo " ... building the images"
	if [ "$FULLARCH" = "i386" ] && [ "$CDIMAGE_INSTALL" = 1 ] && \
	   [ "$CDIMAGE_DVD" != 1 ] && [ "$DIST" != warty ] && \
	   [ "$SPECIAL" != 1 ] && [ "$CDIMAGE_NOSOURCE" != 1 ]; then
		make list $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE

		export OUT="$TMP_OUT/$FULLARCH"; mkdir -p $OUT
		make bin-official_images $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE
		echo Generating MD5Sums of the images
		make imagesums
		echo Generating list files for images
		make pi-makelist

		export OUT="$TMP_OUT/src"; mkdir -p $OUT
		make src-official_images $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE
		echo Generating MD5Sums of the images
		make imagesums
		echo Generating list files for images
		make pi-makelist
	else
		make bin-list $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE
		export OUT=$TMP_OUT/$FULLARCH; mkdir -p $OUT
		make bin-official_images $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE
		if [ $? -gt 0 ]; then
			echo "ERROR WHILE BUILDING OFFICIAL IMAGES !!" >&2
			if [ "$ATTEMPT_FALLBACK" = "yes" ]; then
				echo "I'll try to build a simple (non-bootable) CD" >&2
				make clean
				make installtools
				make bin-images $SIZE_ARGS SRCSIZELIMIT=$FULL_SIZE
			else
				# exit 1
				continue
			fi
		fi
		echo Generating MD5Sums of the images
		make imagesums
		echo Generating list files for images
		make pi-makelist
	fi
	echo "--------------- `date` ---------------"
done
