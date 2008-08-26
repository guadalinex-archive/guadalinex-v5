import gtk
import gtk.gdk
import gtk.glade
import gnome.ui
import gnomevfs

class DiskPreviewUtils:

    def __init__(self):
        self.iconTheme = gtk.icon_theme_get_default()
        self.thumbFactory = gnome.ui.ThumbnailFactory(gnome.ui.THUMBNAIL_SIZE_LARGE)
        self.icon_flags = gnome.ui.ICON_LOOKUP_FLAGS_NONE

    def get_file_pixbuf(self, url, tam=48):
        mime = gnomevfs.get_mime_type(url)
        try:
            info = gnomevfs.get_file_info(url)
            icon_type, result = gnome.ui.icon_lookup(self.iconTheme, self.thumbFactory,
                                                url, "", self.icon_flags, mime, info)
        except:
            print "Error loading icon"
            icon_type = None

        try:
            pixbuf = self.iconTheme.load_icon(icon_type, tam, 0)
        except:
            return None
        return pixbuf

    def get_dir_pixbuf(self, tam=48):
        icon_type = "gnome-fs-directory"
        try:
            pixbuf = self.iconTheme.load_icon(icon_type, tam, 0)
        except:
            return None
        return pixbuf

    def get_harddisk_icons(self):
        icon_type = "drive-harddisk"
        try:
            big_hd_pixbuf = self.iconTheme.load_icon(icon_type, 32, 0)
        except:
            big_hd_pixbuf = None

        try:
            hd_part_pixbuf = self.iconTheme.load_icon(icon_type, 24, 0)
        except:
            hd_part_pixbuf = None

        return (big_hd_pixbuf, hd_part_pixbuf)
