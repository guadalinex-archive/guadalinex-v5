SUBDIRS = ui

mobilemanagerdir = $(pythondir)/MobileManager
mobilemanager_PYTHON = 	__init__.py			\
			MobileController.py		\
			MobileDialPPP.py		\
			MobileDial.py			\
			MobileDevice.py			\
			MobileDeviceHuawei.py		\
			MobileDeviceBluetooth.py	\
			MobileDeviceOption.py		\
			MobileDeviceUSB.py		\
			MobileDeviceSerial.py		\
			MobileStatus.py			\
			MobileCapabilities.py


INCLUDES = 	-I$(top_srcdir)			\
		-I$(top_srcdir)/src		\
		$(PYTHON_CFLAGS)

pkgpyexecdir = $(pythondir)/MobileManager

pkgpyexec_LTLIBRARIES = mdpc.la

mdpc_la_SOURCES = mdpcmodule.c
mdpc_la_LDFLAGS =  -module -avoid-version -export-symbols-regex initmdpc
mdpc_la_LIBADD = $(PYTHON_LIBS)
