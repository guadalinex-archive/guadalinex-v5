udevdir = $(sysconfdir)/udev/rules.d

if HAVE_UDEV_SUSE_RULES
udev_DATA = 04-huawei.rules
endif

if HAVE_UDEV_REDHAT_RULES
udev_DATA = 04-huawei.rules
endif

if HAVE_UDEV_DEBIAN_RULES
udev_DATA = 50-huawei.rules
endif

INCLUDES = $(HUAWEI220_CFLAGS)

sbin_PROGRAMS = huaweiAktBbo1003 huaweiAktBbo1004

huaweiAktBbo1004_LDADD = $(HUAWEI220_LIBS)
huaweiAktBbo1004_SOURCES = huaweiAktBbo-pid-1004.c

huaweiAktBbo1003_LDADD = $(HUAWEI220_LIBS)
huaweiAktBbo1003_SOURCES = huaweiAktBbo-pid-1003.c


EXTRA_DIST= 50-huawei.rules 04-huawei.rules 
