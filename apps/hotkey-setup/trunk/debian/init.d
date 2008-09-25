#!/bin/sh
### BEGIN INIT INFO
# Provides:          hotkey-setup
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      1
# Short-Description: Set up laptop keys to generate keycodes.
### END INIT INFO

# do not run if package is not installed
test -x /usr/sbin/dumpkeycodes || exit 0

manufacturer=`dmidecode --string system-manufacturer`
name=`dmidecode --string system-product-name`
version=`dmidecode --string system-version`

SAVED_STATE=/var/run/hotkey-setup
THINKPAD_LOCKFILE=$SAVED_STATE.thinkpad-keys
THINKPAD_PROC_HOTKEY=/proc/acpi/ibm/hotkey
THINKPAD_KEYS=/usr/sbin/thinkpad-keys

xorg_driver() {
    if [ -e /etc/X11/xorg.conf ] ; then
	sed -n -e '/^[ \t]*section[ \\t]*"device"/I,/^[ \t]*endsection/I{/^[ \t]*driver[ \t]*/I{s/^[ \t]*driver[ \t]*"*//I;s/"*[ \t]*$//;p}}' /etc/X11/xorg.conf | uniq
    fi
}

do_video () {
    for VIDEO in `xorg_driver`; do
        case $VIDEO in
            intel|ati|radeon)
                for x in /proc/acpi/video/*/DOS; do
                    if [ -e "$x" ]; then
                        echo -n 4 >$x;
                    fi
                done
            ;;
        esac
    done

    # Display controller [0380]: Intel Corporation Mobile GM965/GL960 Integrated Graphics Controller [8086:2a03] (rev 0c)
    LSPCI=`lspci -nd 8086:2a03`; 
    if [ -n "$LSPCI" ]; then
        for x in /proc/acpi/video/*/DOS; do
            if [ -e "$x" ]; then
                echo -n 7 >$x; # '4' still let's it crash
            fi
        done
    fi;
}

# This is here because it needs to be executed both if we have a
# Lenovo that also IDs as a ThinkPad, or if we have a real IBM one.
do_thinkpad () {
    . /usr/share/hotkey-setup/ibm.hk
    modprobe thinkpad-acpi

    for VIDEO in `xorg_driver`; do 
        case $VIDEO in
            intel|ati|radeon)
	        echo 0xffffff > $THINKPAD_PROC_HOTKEY
	    ;;
        esac
    done

    # Try to enable the top 8-bits of the mask
    sed -e '/mask:/s/.*\(....\)$/0xff\1/p;d' $THINKPAD_PROC_HOTKEY > $THINKPAD_PROC_HOTKEY
    # If the top bit (ThinkPad key) sticks, skip the polling-daemon
    if ! grep -q '0x[8-9a-f].....$' $THINKPAD_PROC_HOTKEY && test -x $THINKPAD_KEYS; then
        if [ ! -c /dev/input/uinput ]; then
            modprobe uinput
        fi
        if [ ! -c /dev/nvram ]; then
            modprobe nvram
        fi
        $THINKPAD_KEYS $1 && touch $THINKPAD_LOCKFILE
    fi
}

case "$1" in
    start)

# This entire block does nothing on desktops right now
    if laptop-detect; then

    do_video

    /usr/sbin/dumpkeycodes >$SAVED_STATE
    
    if [ $? -gt 0 ]; then
	rm -f $SAVED_STATE
    fi

    . /usr/share/hotkey-setup/key-constants

    case "$manufacturer" in
	Acer*)
	. /usr/share/hotkey-setup/acer.hk
	case "$name" in
	    Aspire\ 16*)
	    . /usr/share/hotkey-setup/acer-aspire-1600.hk
	    ;;
	esac
	;;

	ASUS*)
	. /usr/share/hotkey-setup/asus.hk
	;;

	Compaq*)
	case "$name" in
	    Armada*E500*|Evo*N620*)
	    . /usr/share/hotkey-setup/compaq.hk
	    ;;
	esac
	;;

	Dell*)
	. /usr/share/hotkey-setup/dell.hk
	;;

	Hewlett-Packard*)
	# Load this _first_, so that it can be overridden
	. /usr/share/hotkey-setup/hp.hk
	case "$name" in
	    # Please open a bug if uncommenting these "Presario" entries works for you...
	    #*Presario\ V2000*)
	    #. /usr/share/hotkey-setup/hp-v2000.hk
	    #;;
	    *Tablet*|*tc*)
	    . /usr/share/hotkey-setup/hp-tablet.hk
	    ;;
	esac
	;;

	IBM*)
	do_thinkpad IBM
	;;

	LENOVO*)
	case "$version" in
	    *Think[Pp]ad*)
	    do_thinkpad --no-brightness
	    ;;
	    *)
	    . /usr/share/hotkey-setup/lenovo.hk
	    ;;
	esac
	;;
	
	MEDION*)
	case "$name" in
	    *FID2060*)
	    . /usr/share/hotkey-setup/medion-md6200.hk
	    ;;
	esac
	;;

	MICRO-STAR*)
	case "$name" in
	    *INFINITY*)
	    . /usr/share/hotkey-setup/micro-star-infinity.hk
	    ;;
	esac
	;;

	Samsung*|SAMSUNG*)
	. /usr/share/hotkey-setup/samsung.hk
	;;

	Sony*)
	modprobe sonypi; # Needed to get hotkey events
	modprobe sony-laptop
	;;

	*)
	. /usr/share/hotkey-setup/default.hk	
    esac
    . /usr/share/hotkey-setup/generic.hk
    fi
    ;;
    stop)
	if [ -f $THINKPAD_LOCKFILE ]; then
	    kill `pidof thinkpad-keys` || true ; rm -f $THINKPAD_LOCKFILE
	fi
	if [ -f $SAVED_STATE ]; then
		setkeycodes $(cat $SAVED_STATE) || true
	fi
    ;;
    restart|force-reload)
    $0 stop || true
    $0 start
    ;;
esac
