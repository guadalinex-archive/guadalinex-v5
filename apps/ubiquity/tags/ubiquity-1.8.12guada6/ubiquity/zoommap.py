#! /usr/bin/env python
#
# Gtk Widget With Zoomed Map Selector
# Copyright (C) 2008 Agostino Russo
# Copyright (C) 2008 Canonical Ltd.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
import datetime
import sys
import gobject
import pango
import gtk
import gettext
from math import pi
from gtk import gdk
import ubiquity.tz

# The width, in percent of the allocation, of the hover-to-move areas.
MOTION_AREA = 0.12
# The distance, in pixels, to step when moving.
MOTION_STEP = 20

if gtk.pygtk_version < (2, 8):
    print "PyGtk 2.8 or later required"
    raise SystemExit

try:
    import cairo
except ImportError:
    raise SystemExit("cairo required")

class ZoomMapException(Exception):
    pass

class HotSpot:
    def __init__(self, tz, x, y, parent):
        self.tz = tz
        self.x = float(x)
        self.y = float(y)
        self.selected = False
        # FIXME evand 2008-02-18: something a bit more accurate.
        self.width, self.height = (50, 50)

class ZoomMapWidget(gtk.Widget):
    __gsignals__ = {
        "hotspot_selected" : (gobject.SIGNAL_RUN_FIRST,
                             gobject.TYPE_NONE,
                             (gobject.TYPE_PYOBJECT,))
    }

    def __init__(self, frontend, pixmap, full_zoom, font_selected, font_unselected):
        gtk.Widget.__init__(self)
        self.frontend = frontend
        self.full_zoom = full_zoom
        self.font_selected = gtk.gdk.color_parse(font_selected)
        self.font_unselected = gtk.gdk.color_parse(font_unselected)
        self.load_pixmap(pixmap)
        self.cursor_x = self.cursor_y = None
        self.zoom_window_alllocation = (0,0,0,0)
        self.map_window_alllocation = (0,0,0,0)
        self.update_timeout = None
        self.location_selected = None
        self.tzdb = ubiquity.tz.Database()
        self.lit = False
        self.start_x = 0
        self.start_y = 0
        
        timezone_city_combo = self.frontend.timezone_city_combo

        renderer = gtk.CellRendererText()
        timezone_city_combo.pack_start(renderer, True)
        timezone_city_combo.add_attribute(renderer, 'text', 0)
        list_store = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_STRING)
        timezone_city_combo.set_model(list_store)

        prev_continent = ''
        self.hotspots = []
        for location in self.tzdb.locations:
            # Convert longitude and latitude to percentages of total width and
            # length.
            x = (location.longitude + 180) / 360
            y = 1 - ((location.latitude + 90) / 180)
            hotspot = HotSpot(location, x, y, parent=self)
            self.hotspots.append(hotspot)

            zone_bits = location.zone.split('/')
            if len(zone_bits) == 1:
                continue
            continent = zone_bits[0]
            if continent != prev_continent:
                list_store.append(['', None])
                list_store.append(["--- %s ---" % continent, None])
                prev_continent = continent
            human_zone = '/'.join(zone_bits[1:]).replace('_', ' ')
            list_store.append([human_zone, location.zone])

        self.frontend.timezone_map_window.add(self)

        timezone_city_combo.connect("changed", self.city_changed)
        self.connect("button_release_event", self.button_release)
        self.motion_notify_id = None
        self.leave_notify_id = None
        self.connect("enter_notify_event", self.enter_event)
        self.connect("leave_notify_event", self.leave_event)
        self.connect("map-event", self.mapped)
        self.connect("unmap-event", self.unmapped)
    
    def update_current_time(self):
        if self.location_selected is not None:
            try:
                now = datetime.datetime.now(self.location_selected.tz.info)
                self.frontend.timezone_time_text.set_text(now.strftime('%X'))
            except ValueError:
                # Some versions of Python have problems with clocks set
                # before the epoch (http://python.org/sf/1646728).
                self.frontend.timezone_time_text.set_text('<clock error>')

    def scroll_map(self):
        if not self.allocation:
            return
        x, y, w, h = self.allocation

        left = right = bottom = top = False
        if self.start_x >= 0:
            self.start_x = 0
        else:
            left = True
        if self.start_y >= 0:
            self.start_y = 0
        else:
            top = True
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        if self.start_x <= (-map_w + w):
            self.start_x = (-map_w + w)
        else:
            right = True
        if self.start_y <= (-map_h + h):
            self.start_y = (-map_h + h)
        else:
            bottom = True

        self.zoom_window_alllocation = (0, 0, w, h)
        self.map_window_alllocation = (-self.start_x, -self.start_y, w, h)
        
        cr = self.window.cairo_create()
        self.context = cr
        cr.set_source_pixbuf(self.big_pixbuf, self.start_x, self.start_y)
        cr.paint()

        cr.set_line_width(10)
        cr.set_source_rgba(0.0, 0.0, 0.0, 0.5)
        cr.set_line_join(cairo.LINE_JOIN_MITER)

        if top:
            cr.move_to((w/2)-10, 20)
            cr.rel_line_to(10, -10)
            cr.rel_line_to(10, 10)
            cr.stroke()
        if bottom:
            cr.move_to((w/2)-10,h-20)
            cr.rel_line_to(10, 10)
            cr.rel_line_to(10, -10)
            cr.stroke()
        if left:
            cr.move_to(20, (h/2)-10)
            cr.rel_line_to(-10, 10)
            cr.rel_line_to(10, 10)
            cr.stroke()
        if right:
            cr.move_to(w-20, (h/2)-10)
            cr.rel_line_to(10, 10)
            cr.rel_line_to(-10, 10)
            cr.stroke()

        self.draw_hotspots()

    def timeout(self):
        self.update_current_time()
        self.blink()
        if not self.cursor_x or not self.cursor_y:
            return True
        x, y, w, h = self.allocation
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        scrolling = False
        # right
        if w - self.cursor_x < int(MOTION_AREA * w) and self.start_x > (-map_w + w):
            self.start_x = self.start_x - MOTION_STEP
            scrolling = True
        # left
        elif self.cursor_x < int(MOTION_AREA * w) and self.start_x < 0:
            self.start_x = self.start_x + MOTION_STEP
            scrolling = True
        # top
        if self.cursor_y < int(MOTION_AREA * h) and self.start_y < 0:
            self.start_y = self.start_y + MOTION_STEP
            scrolling = True
        # bottom
        elif h - self.cursor_y < int(MOTION_AREA * h) and self.start_y > (-map_h + h):
            self.start_y = self.start_y - MOTION_STEP
            scrolling = True
        if scrolling:
            self.scroll_map()
        return True

    def mapped(self, widget, event):
        if self.update_timeout is None:
            self.update_timeout = gobject.timeout_add(100, self.timeout)

    def unmapped(self, widget, event):
        if self.update_timeout is not None:
            gobject.source_remove(self.update_timeout)
            self.update_timeout = None

    def enter_timeout(self):
        if not self.allocation:
            return False
        x, y, w, h = self.allocation
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        cursor_x, cursor_y = self.get_pointer()
        if cursor_x >= 0 and cursor_x < w and cursor_y >= 0 and cursor_y < h:
            self.cursor_x = cursor_x
            self.cursor_y = cursor_y
        else:
            return False

        if self.cursor_x < int(MOTION_AREA * w):
            self.start_x = 0
        elif w - self.cursor_x < int(MOTION_AREA * w):
            self.start_x = (-map_w + w)
        else:
            map_x = 1.0 * self.cursor_x / w * map_w
            map_x_offset = min(map_w - w / 2.0, max(map_x - w/2.0, 0.0)) - x
            self.start_x = -map_x_offset
        if self.cursor_y < int(MOTION_AREA * h):
            self.start_y = 0
        elif h - self.cursor_y < int(MOTION_AREA * h):
            self.start_y = (-map_h + h)
        else:
            map_y = 1.0 * self.cursor_y / h * map_h
            map_y_offset = min(map_h - h / 2.0, max(map_y - h/2.0, 0.0)) - y
            self.start_y = -map_y_offset

        if self.motion_notify_id is None:
            self.motion_notify_id = self.connect("motion_notify_event",
                                                 self.motion_notify)
        self.scroll_map()
        self.redraw_zoom_window()
        return False

    def enter_event(self, widget, event):
        if self.leave_notify_id and self.cursor_x:
            gobject.source_remove(self.leave_notify_id)
            return True
        gobject.timeout_add(1000, self.enter_timeout)
        return True

    def leave_timeout(self):
        self.cursor_x = None
        self.cursor_y = None
        if self.motion_notify_id is not None:
            self.disconnect(self.motion_notify_id)
            self.motion_notify_id = None
        self.redraw_all()

    def leave_event(self, widget, event):
        self.leave_notify_id = gobject.timeout_add(2000, self.leave_timeout)
        return True

    def load_pixmap(self, pixmap_filename):
        try:
            self.pixbuf = gtk.gdk.pixbuf_new_from_file(pixmap_filename)
        except:
            raise ZoomMapException("Cannot load the pixmap file %s" % pixmap_filename)

    def button_release(self,widget,event):
        self.hit_test(event.x, event.y)

    def motion_notify(self,widget,event):
        self.cursor_x = event.x
        self.cursor_y = event.y
        self.redraw_zoom_window()

    def redraw_all(self):
        rect = gtk.gdk.Rectangle(*self.allocation)
        self.window.invalidate_rect(rect, True)
        #self.window.process_updates(True)

    def redraw_zoom_window(self):
        x, y, w, h = self.zoom_window_alllocation
        rect = gtk.gdk.Rectangle(int(x)-10, int(y)-10, int(w)+20, int(h)+20)
        self.window.invalidate_rect(rect, True)
        #self.window.process_updates(True)

    def do_realize(self):
        self.set_flags(self.flags() | gtk.REALIZED)
        self.window = gdk.Window(
            self.get_parent_window(),
            width=self.allocation.width,
            height=self.allocation.height,
            window_type=gdk.WINDOW_CHILD,
            wclass=gdk.INPUT_OUTPUT,
            event_mask=self.get_events() |
                        gdk.EXPOSURE_MASK |
                        gdk.ENTER_NOTIFY_MASK |
                        gdk.LEAVE_NOTIFY_MASK |
                        gdk.BUTTON_PRESS_MASK |
                        gdk.BUTTON_RELEASE_MASK |
                        gdk.POINTER_MOTION_MASK)
        self.window.set_user_data(self)
        self.style.attach(self.window)
        self.style.set_background(self.window, gtk.STATE_NORMAL)
        self.window.move_resize(*self.allocation)

    def do_unrealize(self):
        self.window.set_user_data(None)

    def do_size_request(self, requisition):
        pass

    def do_size_allocate(self, allocation):
        self.allocation = allocation
        if self.flags() & gtk.REALIZED:
            self.window.move_resize(*allocation)
        x,y,w,h = allocation
        self.small_pixbuf = self.pixbuf.scale_simple(w, h, gtk.gdk.INTERP_BILINEAR)
        self.big_pixbuf = self.pixbuf.scale_simple(int(w * 4.5), int(h * 4.5), gtk.gdk.INTERP_BILINEAR)

    def do_expose_event(self, event):
        self.context = self.window.cairo_create()
        self.context.rectangle(event.area.x, event.area.y,
                           event.area.width, event.area.height)
        self.context.clip()
        if not self.cursor_x and not self.cursor_y:
            self.draw_map()
            return
        if not self.full_zoom:
            self.draw_map()
            self.draw_zoom_window()
        else:
            self.scroll_map()

    def draw_map(self):
        x, y, w, h = self.allocation
        cr = self.context
        map_w = self.small_pixbuf.get_width()
        map_h = self.small_pixbuf.get_height()
        cr.set_source_pixbuf(self.small_pixbuf, 0, 0)
        cr.paint()
        for hotspot in self.hotspots:
            cr.set_source_color(gtk.gdk.color_parse("black"))
            cr.arc(hotspot.x*w, hotspot.y*h, 2, 0, 2*pi)
            cr.fill()
            cr.arc(hotspot.x*w, hotspot.y*h, 1, 0, 2*pi)
            if self.location_selected and hotspot == self.location_selected:
                cr.set_source_color(self.font_selected)
            else:
                cr.set_source_color(self.font_unselected)
            cr.fill()
            cr.stroke()

    def blink(self):
        if not self.location_selected:
            return
        cr = self.window.cairo_create()
        if self.cursor_x and self.cursor_y:
            map_w = self.big_pixbuf.get_width()
            map_h = self.big_pixbuf.get_height()
            x1 = map_w * self.location_selected.x
            y1 = map_h * self.location_selected.y
            min_x, min_y, max_w, max_h = self.map_window_alllocation
            offset_x, offset_y, xx, yy = self.zoom_window_alllocation
            x2 = offset_x + x1 - min_x
            y2 = offset_y + y1 - min_y
            cr.set_source_color(gtk.gdk.color_parse("black"))
            cr.arc(x2, y2, 2, 0, 2*pi)
            cr.fill()
            x = x2
            y = y2
        else:
            x, y, w, h = self.allocation
            x = self.location_selected.x * w
            y = self.location_selected.y * h
            cr.set_source_color(gtk.gdk.color_parse("black"))
            cr.arc(self.location_selected.x*w, self.location_selected.y*h, 2, 0, 2*pi)
            cr.fill()
        if self.lit:
            cr.set_source_color(self.font_selected)
        else:
            cr.set_source_color(self.font_unselected)
        self.lit = not self.lit
        cr.arc(x, y, 1, 0, 2*pi)
        cr.fill()
        cr.stroke()

    def draw_zoom_window(self):
        if not self.cursor_x and not self.cursor_y: return
        if not self.cursor_x: self.cursor_x = 0
        if not self.cursor_y: self.cursor_y = 0
        cr = self.context
        x, y, widget_w, widget_h = self.allocation
        w = widget_w/3.0
        h = widget_h/3.0
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        x = max(1.0*self.cursor_x - w/2.0 , 0.0)
        y = max(1.0*self.cursor_y - h/2.0 , 0.0)
        map_x = 1.0*self.cursor_x/widget_w *map_w
        map_y = 1.0*self.cursor_y/widget_h * map_h
        map_x_offset = min(map_w - w/2.0, max(map_x - w/2.0, 0.0)) - x
        map_y_offset = min(map_h - h/2.0, max(map_y - h/2.0, 0.0)) - y
        self.zoom_window_alllocation =  (x, y, w, h)
        self.map_window_alllocation =  (map_x_offset+x, map_y_offset+y, w, h)
        cr.rectangle(*self.zoom_window_alllocation)
        cr.set_source_pixbuf(self.big_pixbuf, -map_x_offset, -map_y_offset)
        cr.fill_preserve()
        cr.set_source_color(self.style.fg[self.state])
        cr.set_line_width(2.0)
        cr.stroke_preserve()
        cr.clip()

    def nearest_hotspot(self, cursor_x, cursor_y):
        if not cursor_x or not cursor_y:
            return None
        x, y, w, h = self.allocation
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        min_x, min_y, max_w, max_h = self.map_window_alllocation
        offset_x, offset_y, xx, yy = self.zoom_window_alllocation
        best_hotspot = None
        best_distance = None
        for hotspot in self.hotspots:
            x1 = map_w * hotspot.x
            y1 = map_h * hotspot.y
            x2 = offset_x + x1 - min_x
            y2 = offset_y + y1 - min_y
            if x1 < min_x or x1 > min_x + max_w or y1 < min_y or y1 > min_y + max_h: continue
            if (abs(cursor_x - x2) < hotspot.width and abs(cursor_y - y2) < hotspot.height):
                distance = ((cursor_x - x2) ** 2 + (cursor_y - y2) ** 2) ** 0.5
                if best_distance is None or distance < best_distance:
                    best_hotspot = hotspot
                    best_distance = distance
        return best_hotspot

    def draw_hotspots(self):
        if not self.cursor_x and not self.cursor_y: return
        x, y, w, h = self.allocation
        map_w = self.big_pixbuf.get_width()
        map_h = self.big_pixbuf.get_height()
        min_x, min_y, max_w, max_h = self.map_window_alllocation
        offset_x, offset_y, xx, yy = self.zoom_window_alllocation
        cr = self.context
        best_hotspot = self.nearest_hotspot(self.cursor_x, self.cursor_y)
        for hotspot in self.hotspots:
            x1 = map_w * hotspot.x
            y1 = map_h * hotspot.y
            x2 = offset_x + x1 - min_x
            y2 = offset_y + y1 - min_y
            if x1 < min_x or x1 > min_x + max_w or y1 < min_y or y1 > min_y + max_h: continue
            cr.move_to(x2, y2)
            
            cr.set_source_color(gtk.gdk.color_parse("black"))
            cr.arc(x2, y2, 2, 0, 2*pi)
            cr.fill()

            if (self.location_selected and hotspot == self.location_selected):
                cr.set_source_color(self.font_selected)
            elif best_hotspot is not None and hotspot == best_hotspot:
                cr.set_source_color(self.font_selected)
            else:
                cr.set_source_color(self.font_unselected)
            cr.arc(x2, y2, 1, 0, 2*pi)
            cr.fill()
            cr.stroke()

    def hit_test(self, cursor_x, cursor_y):
        best_hotspot = self.nearest_hotspot(cursor_x, cursor_y)
        if best_hotspot is not None:
            if (not self.location_selected or
                best_hotspot != self.location_selected):
                self.select_hotspot(best_hotspot)
                self.set_city_text(best_hotspot.tz.zone)
                self.set_zone_text(best_hotspot.tz)

    def select_hotspot(self, hotspot):
        if not isinstance(hotspot, HotSpot):
            raise ZoomMapException("Invalid hotspot %s" % hotspot)
        self.location_selected = hotspot
        self.redraw_all()
        self.emit("hotspot_selected", hotspot)

    def selected_hotspot(self):
        return self.location_selected
   
    def city_changed(self, widget):
        iterator = widget.get_active_iter()
        if iterator is not None:
            model = widget.get_model()
            location = model.get_value(iterator, 1)
            if location is not None:
                self.set_tz_from_name(location)
    
    def set_tz_from_name(self, location):
        hotspot = None
        for h in self.hotspots:
            if h.tz.zone == location:
                hotspot = h
                break
        else:
            return

        # FIXME evand 2008-02-18:
        #self.select_hotspot(hotspot)
        self.location_selected = hotspot
        self.set_city_text(hotspot.tz.zone)
        self.set_zone_text(hotspot.tz)

    def get_selected_tz_name(self):
        if self.location_selected is not None:
            return self.location_selected.tz.zone
        else:
            return None

    def set_city_text(self, name):
        model = self.frontend.timezone_city_combo.get_model()
        iterator = model.get_iter_first()
        while iterator is not None:
            location = model.get_value(iterator, 1)
            if location == name:
                self.frontend.timezone_city_combo.set_active_iter(iterator)
                break
            iterator = model.iter_next(iterator)

    def set_zone_text(self, location):
        offset = location.utc_offset
        if offset >= datetime.timedelta(0):
            minuteoffset = int(offset.seconds / 60)
        else:
            minuteoffset = int(offset.seconds / 60 - 1440)
        if location.zone_letters == 'GMT':
            text = location.zone_letters
        else:
            text = "%s (GMT%+d:%02d)" % (location.zone_letters,
                                         minuteoffset / 60, minuteoffset % 60)
        self.frontend.timezone_zone_text.set_text(text)
        translations = gettext.translation('iso_3166',
                                           languages=[self.frontend.locale],
                                           fallback=True)
        self.frontend.timezone_country_text.set_text(
            translations.ugettext(location.human_country))
        self.update_current_time()

gobject.type_register(ZoomMapWidget)
