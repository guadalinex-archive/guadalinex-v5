bin_SCRIPTS = mount-systray

mountsystraydir = $(prefix)/share/mount-systray
mountsystray_PYTHON = \
	mount-systray.py \
	config.py \
	MSDeviceManager.py \
	MSSystray.py \
	MSConfig.py

DISTCLEANFILES = \
	$(bin_SCRIPTS) \
	config.py

EXTRA_DIST =  \
	$(bin_SCRIPTS) \
	$(mountsystray_PYTHON)
