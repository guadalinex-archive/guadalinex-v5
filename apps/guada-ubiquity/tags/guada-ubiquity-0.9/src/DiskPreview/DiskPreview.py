import gtk
import gtk.gdk
import gtk.glade
import gnome.ui
import gnomevfs
import os
import dbus
import popen2
import DiskPreviewUtils

class DiskPreview(gtk.VBox):
    def __init__(self):
        gtk.VBox.__init__(self,2)
        self.cursor = None
        self.mounted_list = []
        self.path_first_button = ""
        self.path_second_button = ""
        self.path_third_button = ""
        
        #Dbus stuff
        self.bus = dbus.SystemBus()
        self.hal_manager_obj = self.bus.get_object("org.freedesktop.Hal", "/org/freedesktop/Hal/Manager")
        self.hal_manager = dbus.Interface(self.hal_manager_obj,"org.freedesktop.Hal.Manager")

        #DiskPreviewUtils
        self.utils = DiskPreviewUtils.DiskPreviewUtils()

        #Glade stuff
        GLADEDIR = "/usr/share/ubiquity/glade/"
        self.xml = gtk.glade.XML(GLADEDIR + "diskpreview.glade", root="disk_preview_main_hbox")
        
        self.disk_preview_main_hbox = self.xml.get_widget("disk_preview_main_hbox")
        self.disk_preview_sidebar_vbox = self.xml.get_widget("disk_preview_sidebar_vbox")
        self.disk_preview_browser_vbox = self.xml.get_widget("disk_preview_browser_vbox")
        self.disk_preview_treeview = self.xml.get_widget("disk_preview_treeview")
        self.disk_preview_fs_label = self.xml.get_widget("disk_preview_fs_label")
        self.disk_preview_free_label = self.xml.get_widget("disk_preview_free_label")
        self.disk_preview_used_label = self.xml.get_widget("disk_preview_ocu_label")
        self.disk_preview_total_label = self.xml.get_widget("disk_preview_tam_label")
        self.disk_preview_info_panel = self.xml.get_widget("disk_preview_info_panel")
        self.disk_preview_info_panel.hide()
        self.disk_preview_info_panel.set_no_show_all(True)
        self.disk_preview_path_hbox = self.xml.get_widget("disk_preview_path_hbox")
        self.disk_preview_path_hbox.hide()
        self.disk_preview_path_hbox.set_no_show_all(True)
        self.disk_preview_back_button = self.xml.get_widget("disk_preview_back_button")
        self.disk_preview_harddisk_button = self.xml.get_widget("disk_preview_harddisk_button")
        self.disk_preview_first_dir_button = self.xml.get_widget("disk_preview_first_dir_button")
        self.disk_preview_first_dir_label = self.xml.get_widget("disk_preview_first_dir_label")
        self.disk_preview_second_dir_button = self.xml.get_widget("disk_preview_second_dir_button")
        self.disk_preview_second_dir_label = self.xml.get_widget("disk_preview_second_dir_label")
        self.disk_preview_third_dir_button = self.xml.get_widget("disk_preview_third_dir_button")
        self.disk_preview_third_dir_label = self.xml.get_widget("disk_preview_third_dir_label")
        self.disk_preview_forward_button = self.xml.get_widget("disk_preview_forward_button")
        
        self.disk_preview_browser_iconview = self.xml.get_widget("disk_preview_browser_iconview")
        
	self.disk_preview_browser_model = gtk.ListStore (str, gtk.gdk.Pixbuf, str)
        self.disk_preview_browser_iconview.set_model(self.disk_preview_browser_model)
        self.disk_preview_browser_iconview.set_text_column(0)
        self.disk_preview_browser_iconview.set_pixbuf_column(1)

        # self.disk_preview_treeview_model :
        #
        # gtk.gdk.Pixbuf : hard disk icon
        # str : Name of the hard disk or partition
        # str : Total partition space
        # str : Ocupated partition space 
        # str : Free partition space
        # str : File system type
        # str : dev path
        # str : path of the mounted filesystem
        self.disk_preview_treeview_model = gtk.TreeStore(gtk.gdk.Pixbuf, str, str, str, str, str, str, str)
        self.disk_preview_treeview.set_model(self.disk_preview_treeview_model)
        disk_preview_treeview_sort = gtk.TreeModelSort(self.disk_preview_treeview_model)
        disk_preview_treeview_sort.set_sort_column_id(1, gtk.SORT_ASCENDING)
        
        cellrenderpixbuf = gtk.CellRendererPixbuf()
        cellrendertext = gtk.CellRendererText()
        column = gtk.TreeViewColumn('hd_icon',
                                    cellrenderpixbuf,
                                    pixbuf=0)
        self.disk_preview_treeview.append_column(column)
        column = gtk.TreeViewColumn('hd_name',
                                    cellrendertext,
                                    markup=1)
        self.disk_preview_treeview.append_column(column)

        self.pack_start(self.disk_preview_main_hbox, expand=True)

        selection = self.disk_preview_treeview.get_selection()
        selection.set_mode(gtk.SELECTION_SINGLE)
 
        #Signals
        selection.connect("changed", self.__disk_preview_treview_row_changed_cb, None)
        self.disk_preview_browser_iconview.connect("item-activated", self.__disk_preview_browser_iconview_item_activated_cb, None)
        self.disk_preview_harddisk_button.connect("clicked", self.__disk_preview_harddisk_button_cb, None)
        self.fdb_handler_id = self.disk_preview_first_dir_button.connect("toggled", self.__disk_preview_first_dir_button_cb, None)
        self.sdb_handler_id = self.disk_preview_second_dir_button.connect("toggled", self.__disk_preview_second_dir_button_cb, None)
        self.tdb_handler_id = self.disk_preview_third_dir_button.connect("toggled", self.__disk_preview_third_dir_button_cb, None)
        self.disk_preview_back_button.connect("clicked", self.__disk_preview_back_button_cb, None)
        self.disk_preview_forward_button.connect("clicked", self.__disk_preview_forward_button_cb, None)
        
    #Public methods
    def mount_filesystems(self):
        #if len(self.mounted_list) == 0:
        #    return
        
        self.disk_preview_treeview_model.clear()
        self.__mount_filesystems()
        selection = self.disk_preview_treeview.get_selection()
        selection.select_path(0)
        
    def umount_filesystems(self):
        for fs in self.mounted_list :
            os.system ("umount %s" % fs)
            print "umount %s" % fs
        self.mounted_list = []
    

    #Class Callbacks
    def __disk_preview_browser_iconview_item_activated_cb(self, iconview, path, data):
        model = iconview.get_model()
        iter = model.get_iter(path)
        if os.path.isdir(model.get_value(iter, 2)):
            self.__populate_browser_iconview(model.get_value(iter, 2))
        #UNCOMMENT THIS IF YOU WANT OPEN FILES TOO
        #else:
        #    try:
        #        os.system("gnome-open %s" % model.get_value(iter, 2))
        #    except:
        #        return

    def __disk_preview_treview_row_changed_cb(self, selection, data):
        model,pathlist = selection.get_selected_rows()
        
        if len(pathlist) == 0 :
            return
        
        path = pathlist[0]

        if len(path) > 1:
            self.disk_preview_info_panel.set_no_show_all(False)
            self.disk_preview_path_hbox.set_no_show_all(False)
            self.disk_preview_info_panel.show_all()
            self.disk_preview_path_hbox.show_all()
            iter = model.get_iter(path)
            mount_point = model.get_value(iter, 7)
            self.__populate_browser_iconview(mount_point)
            self.__update_disk_info_panel(model.get_value(iter, 2),
                                          model.get_value(iter, 3),
                                          model.get_value(iter, 4),
                                          model.get_value(iter, 5))
        else:
            self.disk_preview_path_hbox.hide_all()
            self.disk_preview_info_panel.hide_all()
            self.disk_preview_browser_iconview.get_model().clear()

    def __disk_preview_harddisk_button_cb(self, widget, data):
        self.__populate_browser_iconview(self.path_hardisk_button)

    def __disk_preview_first_dir_button_cb(self, widget, data):
        self.__populate_browser_iconview(self.path_first_button)
        
    def __disk_preview_second_dir_button_cb(self, widget, data):
        self.__populate_browser_iconview(self.path_second_button)
        
    def __disk_preview_third_dir_button_cb(self, widget, data):
        self.__populate_browser_iconview(self.path_third_button)

    def __disk_preview_back_button_cb(self, widget, data):
        path_list = self.path_first_button.lstrip("/").split("/")

        if len(path_list) == 7:
            self.disk_preview_harddisk_button.show()
            self.disk_preview_back_button.hide()
            self.disk_preview_forward_button.show()
            self.path_third_button = self.path_second_button
            self.path_second_button = self.path_first_button
            self.path_first_button = self.__get_path_from_path_list(path_list, 0, 5)
        else:
            self.disk_preview_harddisk_button.hide()
            self.disk_preview_back_button.show()
            self.disk_preview_forward_button.show()
            self.path_third_button = self.path_second_button
            self.path_second_button = self.path_first_button
            self.path_first_button = self.__get_path_from_path_list(path_list, 0, len(path_list)-2)

        self.__update_dir_buttons()  

    def __disk_preview_forward_button_cb(self, widget, data):
        path_list = self.path_third_button.lstrip("/").split("/")
        full_path_list = self.current_full_path.lstrip("/").split("/") 

        if len(path_list) == len(full_path_list)-1 :
            self.disk_preview_harddisk_button.hide()
            self.disk_preview_back_button.show()
            self.disk_preview_forward_button.hide()
            self.path_first_button = self.path_second_button
            self.path_second_button = self.path_third_button
            tmp_path_list = path_list + [full_path_list[len(path_list)]]
            self.path_third_button = self.__get_path_from_path_list(tmp_path_list, 0, len(path_list))
        else:
            self.disk_preview_harddisk_button.hide()
            self.disk_preview_back_button.show()
            self.disk_preview_forward_button.show()
            self.path_first_button = self.path_second_button
            self.path_second_button = self.path_third_button
            tmp_path_list = path_list + [full_path_list[len(path_list)]]
            self.path_third_button = self.__get_path_from_path_list(tmp_path_list, 0, len(path_list))
   
        self.__update_dir_buttons()      
        
    #Private Methods
    
    def __update_disk_info_panel(self, total, used, free, fs):
            self.disk_preview_fs_label.set_text(fs)
            self.disk_preview_free_label.set_text(free)
            self.disk_preview_used_label.set_text(used)
            self.disk_preview_total_label.set_text(total)

    def __update_dir_buttons(self):
        self.disk_preview_first_dir_button.handler_block(self.fdb_handler_id)
        self.disk_preview_second_dir_button.handler_block(self.sdb_handler_id)
        self.disk_preview_third_dir_button.handler_block(self.tdb_handler_id)
        
        if self.path_first_button == self.current_full_path :
            self.disk_preview_first_dir_label.set_markup("<b>%s</b>" % os.path.basename(self.path_first_button))
            self.disk_preview_first_dir_button.set_active(True)
        else:
            self.disk_preview_first_dir_label.set_markup(os.path.basename(self.path_first_button))
            self.disk_preview_first_dir_button.set_active(False)

        if self.path_second_button == self.current_full_path :
            self.disk_preview_second_dir_label.set_markup("<b>%s</b>" % os.path.basename(self.path_second_button))
            self.disk_preview_second_dir_button.set_active(True)
        else:
            self.disk_preview_second_dir_label.set_markup(os.path.basename(self.path_second_button))
            self.disk_preview_second_dir_button.set_active(False)

        if self.path_third_button == self.current_full_path :
            self.disk_preview_third_dir_label.set_markup("<b>%s</b>" % os.path.basename(self.path_third_button))
            self.disk_preview_third_dir_button.set_active(True)
        else:
            self.disk_preview_third_dir_label.set_markup(os.path.basename(self.path_third_button))
            self.disk_preview_third_dir_button.set_active(False)

        self.disk_preview_first_dir_button.handler_unblock(self.fdb_handler_id)
        self.disk_preview_second_dir_button.handler_unblock(self.sdb_handler_id)
        self.disk_preview_third_dir_button.handler_unblock(self.tdb_handler_id)

    def __get_path_from_path_list(self, path_list, f, t):
        ret = "/"
        for x in range(f, t+1):
            if x != t :
                ret = ret + path_list[x] + "/"
            else:
                ret = ret + path_list[x]
                
        return ret

    def __update_disk_path_area(self, path):
        self.disk_preview_back_button.hide()
        self.disk_preview_harddisk_button.hide()
        self.disk_preview_first_dir_button.hide()
        self.disk_preview_second_dir_button.hide()
        self.disk_preview_third_dir_button.hide()
        self.disk_preview_forward_button.hide()

        path_list = path.lstrip("/").split("/")

        self.path_hardisk_button = self.__get_path_from_path_list(path_list, 0, 4)
        self.current_full_path = path

        if len(path_list) == 5 :
             self.disk_preview_harddisk_button.show()

        if len(path_list) > 5 and len(path_list) <= 8:
            self.disk_preview_harddisk_button.show()
            if len(path_list) == 6:
                self.disk_preview_first_dir_button.show()
                self.path_first_button = self.__get_path_from_path_list(path_list, 0, 5)
                self.path_second_button = ""
                self.path_third_button = ""
            if len(path_list) == 7:
                self.disk_preview_first_dir_button.show()
                self.disk_preview_second_dir_button.show()
                self.path_first_button = self.__get_path_from_path_list(path_list, 0, 5)
                self.path_second_button = self.__get_path_from_path_list(path_list, 0, 6)
                self.path_third_button = ""
            if len(path_list) == 8:
                self.disk_preview_first_dir_button.show()
                self.disk_preview_second_dir_button.show()
                self.disk_preview_third_dir_button.show()
                self.path_first_button = self.__get_path_from_path_list(path_list, 0, 5)
                self.path_second_button = self.__get_path_from_path_list(path_list, 0, 6)
                self.path_third_button = self.__get_path_from_path_list(path_list, 0, 7)


        if len(path_list) > 8:
            self.disk_preview_back_button.show()
            self.disk_preview_first_dir_button.show()
            self.disk_preview_second_dir_button.show()
            self.disk_preview_third_dir_button.show()
            self.path_first_button = self.__get_path_from_path_list(path_list, 0, len(path_list) - 3)
            self.path_second_button = self.__get_path_from_path_list(path_list, 0, len(path_list) - 2)
            self.path_third_button = self.__get_path_from_path_list(path_list, 0, len(path_list) - 1)

        self.__update_dir_buttons()
        
        
    def __try_to_mount_filesystem(self, fstype, dev_path, mount_path):
        if fstype == "swap" or fstype == "":
            return None
        if os.system("mkdir -p %s" % mount_path) == 0:
            if os.system("mount -t %s %s %s" % (fstype, dev_path, mount_path)) == 0:
                print "mount -t %s %s %s" % (fstype, dev_path, mount_path)
                infd ,outfd ,errfd = os.popen3("df -h | grep %s" % mount_path)
                line = outfd.readline().strip("\n").split()
                if len(line) > 0 :
                    self.mounted_list.append(mount_path)
                    return (line[1], line[2], line[3])
                else:
                    return None
            else:
                return None
        else:
            return None
    
    def __mount_filesystems(self):
        big_hd_pixbuf, hd_part_pixbuf = self.utils.get_harddisk_icons()
        disk_dict = {}

################## HAL STUFF ##################
## This stuff doesn't works now because there is a bug in hald running on live cd. This
## bug is that hal doesn't refresh when you change a partition
## You can uncomment this in the future. And comment the rest of the code
##
##         devices = self.hal_manager.GetAllDevices()
##         for dev in devices:
##             device_dbus_obj = self.bus.get_object("org.freedesktop.Hal", dev)
##             dev_info = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
##             if dev_info.has_key("info.category") and dev_info["info.category"] == "volume" :
##                 parent_dbus_obj =  self.bus.get_object("org.freedesktop.Hal", dev_info["info.parent"])
##                 parent_info = parent_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
##                 parent_path = parent_info["block.device"]
##                 if not disk_dict.has_key(parent_path) and parent_info["storage.drive_type"] == "disk" :
##                     hd_description = "<b>%s</b>\n<small>Dispositivo : (%s)</small>" % (parent_info["info.product"], parent_path) 
##                     disk_dict[parent_path] = self.disk_preview_treeview_model.append(None, [big_hd_pixbuf, hd_description , "", "", "", "", "", ""])
##                 dev_path = dev_info["block.device"]
##                 fstype = dev_info["volume.fstype"]
##                 mount_point = os.path.join("/var/tmp/diskpreview/", os.path.basename(parent_path), os.path.basename(dev_path))
##                 if parent_info["storage.drive_type"] == "disk":
##                     tam_info = self.__try_to_mount_filesystem(fstype, dev_path, mount_point)
##                     if tam_info != None:
##                         self.disk_preview_treeview_model.append(disk_dict[parent_path], [hd_part_pixbuf, os.path.basename(dev_path),
##                                                                                          tam_info[0], tam_info[1], tam_info[2],
##                                                                                          fstype, dev_path, mount_point])
################################################

        devices = self.hal_manager.GetAllDevices()
        for dev in devices:
            device_dbus_obj = self.bus.get_object("org.freedesktop.Hal", dev)
            dev_info = device_dbus_obj.GetAllProperties(dbus_interface="org.freedesktop.Hal.Device")
            if dev_info.has_key("block.device") and dev_info.has_key("storage.drive_type") and dev_info["storage.drive_type"] == "disk" :
                hd_description = "<b>%s</b>\n<small>Dispositivo : (%s)</small>" % (dev_info["info.product"], dev_info["block.device"]) 
                disk_dict[dev_info["block.device"]] = self.disk_preview_treeview_model.append(None, [big_hd_pixbuf, hd_description , "", "", "", "", "", ""])
                parts_cmd = popen2.Popen3("ls %s[0-9]*" % dev_info["block.device"])
                parts_cmd.wait()
                for partition in parts_cmd.fromchild.readlines():
                    dev_path = partition.strip("\n")
                    mount_point = os.path.join("/var/tmp/diskpreview/", os.path.basename(dev_info["block.device"]), os.path.basename(dev_path))
                    tam_info = self.__try_to_mount_filesystem("auto", dev_path, mount_point)
                    if tam_info != None:
                        mount_cmd = popen2.Popen3("mount | grep %s" % dev_path)
                        mount_cmd.wait()
                        mount_line = mount_cmd.fromchild.readline()
                        fstp = mount_line.split()[4]
                        self.disk_preview_treeview_model.append(disk_dict[dev_info["block.device"]], [hd_part_pixbuf, os.path.basename(dev_path),
                                                                                                      tam_info[0], tam_info[1], tam_info[2],
                                                                                                      fstp, dev_path, mount_point]) 
        self.disk_preview_treeview.expand_all()
        
            
    def __populate_browser_iconview(self, path="/"):
        self.cursor = gtk.gdk.Cursor(gtk.gdk.WATCH)
        self.window.set_cursor(self.cursor)
        gtk.gdk.flush()
        
        dir_list = os.listdir(path)
        dir_items = []
        file_items = []
        
        for item in dir_list:
            if os.path.isdir(os.path.join(path, item)) :
                dir_items.append(item)
            else:
                file_items.append(item)

        dir_items.sort()
        file_items.sort()

        self.disk_preview_browser_model.clear()
        
        for dir_item in dir_items:
            if not dir_item.startswith("."):
                self.disk_preview_browser_model.append([dir_item, self.utils.get_dir_pixbuf(),
                                                        os.path.join(path, dir_item)])

        for file_item in file_items:
            if not file_item.startswith("."):
                self.disk_preview_browser_model.append([file_item, self.utils.get_file_pixbuf(os.path.join(path, file_item)),
                                                        os.path.join(path, file_item)])
        self.__update_disk_path_area(path)

        
        self.cursor = None
        self.window.set_cursor(self.cursor)
        gtk.gdk.flush()

        
        
def main():
	window = gtk.Window()
	dp = DiskPreview()
        dp.mount_filesystems()
	window.add(dp)
        
	# Conectamos el evento destroy con la salida del bucle de eventos
	window.connect("destroy", gtk.main_quit)
	# Dibujamos toda la ventana
	window.show_all()

	# Comenzamos el bucle de eventos
	gtk.main()
        dp.umount_filesystems()

if __name__ == "__main__":
	main()
