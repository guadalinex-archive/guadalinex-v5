/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * src/properties.c - control panel applet for gnome-volume-manager
 *
 * Robert Love <rml@novell.com>
 * Jeffrey Stedfast <fejj@novell.com>
 *
 * (C) Copyright 2005-2007 Novell, Inc.
 *
 * Licensed under the GNU GPL v2.  See COPYING.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <gnome.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <libhal.h>

#include "gvm.h"

#define GLADE_XML_FILE	"gnome-volume-properties.glade"


typedef enum {
	TYPE_BOOL,
	TYPE_STRING,
	TYPE_COMBO,
	TYPE_SPIN
} type_t;

static struct {
	char *key;
	type_t type;
	GtkWidget *widget;
	gboolean need_daemon;
} gvm_settings[] = {
	{ GCONF_ROOT    "autokeyboard",              TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autokeyboard_command",      TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "automouse",                 TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "automouse_command",         TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autophoto",                 TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autophoto_command",         TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autopalmsync",              TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autopalmsync_command",      TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autopocketpc",              TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autopocketpc_command",      TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autoprinter",               TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autoprinter_command",       TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autoscanner",               TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autoscanner_command",       TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autotablet",                TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autotablet_command",        TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autovideocam",              TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autovideocam_command",      TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT    "autowebcam",                TYPE_BOOL,   NULL, TRUE  },
	{ GCONF_ROOT    "autowebcam_command",        TYPE_STRING, NULL, FALSE },
	{ GCONF_ROOT_MS "click_on_device_action",    TYPE_COMBO,  NULL, FALSE },
	{ GCONF_ROOT_MS "hide_systray",              TYPE_BOOL,   NULL, FALSE },
	{ GCONF_ROOT_MS "blink_all_time",            TYPE_BOOL,   NULL, FALSE },
	{ GCONF_ROOT_MS "minutes_warning",           TYPE_SPIN,   NULL, FALSE },
	{ GCONF_ROOT_MS "show_notify",               TYPE_BOOL,   NULL, FALSE },
	{ GCONF_ROOT_MS "show_notify_detected",      TYPE_BOOL,   NULL, FALSE },
};

static GHashTable *gvm_settings_hash = NULL;
static GConfClient *gconf = NULL;
static GladeXML *xml = NULL;
static gboolean updating = FALSE;
static GtkWidget *props = NULL;

static void
changed_cb (GtkEntry *entry, const char *key)
{
	const char *str;
	
	if (updating)
		return;
	
	str = gtk_entry_get_text (entry);
	gconf_client_set_string (gconf, key, str ? str : "", NULL);
}

static void
toggled_cb (GtkToggleButton *toggle, const char *key)
{
	GtkWidget *hbox;
	gboolean bool;
	char *name;
	
	bool = gtk_toggle_button_get_active (toggle);
	
	if (!updating)
		gconf_client_set_bool (gconf, key, bool, NULL);
	
	name = strrchr (key, '/') + 1;
	name = g_strdup_printf ("%s_hbox", name);
	if ((hbox = glade_xml_get_widget (xml, name)))
		gtk_widget_set_sensitive (hbox, bool);
	g_free (name);
}

static void
change_value_cb (GtkSpinButton *spinbutton, const char *key)
{
	int value;

	if (updating)
		return;

	value = gtk_spin_button_get_value_as_int (spinbutton);
	gconf_client_set_int (gconf, key, value, NULL);
}

static void
changed_combo_cb (GtkComboBox *combo, const char *key)
{
	const char *str;
	GtkTreeIter iter;
	GtkTreeModel *model;

	if (updating)
		return;

	model = gtk_combo_box_get_model (combo);
	gtk_combo_box_get_active_iter (combo, &iter);
	gtk_tree_model_get (model, &iter,
			    0, &str,
			    -1);
	gconf_client_set_string (gconf, key, str ? str : "", NULL);
}

static void
gconf_changed_cb (GConfClient *client GNUC_UNUSED,
		  guint id GNUC_UNUSED,
		  GConfEntry *entry,
		  gpointer data GNUC_UNUSED)
{
	GConfValue *value;
	gpointer result;
	int which;
	
	g_return_if_fail (gconf_entry_get_key (entry) != NULL);
	
	if (!(value = gconf_entry_get_value (entry)))
		return;
	
	if (!(result = g_hash_table_lookup (gvm_settings_hash, entry->key)))
		return;
	
	which = GPOINTER_TO_INT (result) - 1;
	
	updating = TRUE;
	
	if (gvm_settings[which].type == TYPE_STRING) {
		gtk_entry_set_text (GTK_ENTRY (gvm_settings[which].widget), gconf_value_get_string (value));
	} else if (gvm_settings[which].type == TYPE_BOOL) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gvm_settings[which].widget),
					      gconf_value_get_bool (value));
	} else {
		g_assert_not_reached ();
	}
	
	updating = FALSE;
}

static void
set_sensitivity (void)
{
	GtkWidget *hbox;
	gboolean bool;
	size_t i;
	
	/* checkboxes can enable/disable the ability to change other settings */
	for (i = 0; i < G_N_ELEMENTS (gvm_settings); i++) {
		if (gvm_settings[i].type == TYPE_BOOL) {
			/* check if we have dependents */
			char *name;
			
			name = strrchr (gvm_settings[i].key, '/') + 1;
			name = g_strdup_printf ("%s_hbox", name);
			if ((hbox = glade_xml_get_widget (xml, name))) {
				bool = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gvm_settings[i].widget));
				gtk_widget_set_sensitive (hbox, bool);
			}
			g_free (name);
		}
	}
}

/*
 * close_cb - gtk call-back on the "Close" button
 */
static void
close_cb (GtkWidget *dialog,
	  int response GNUC_UNUSED,
	  gpointer user_data GNUC_UNUSED)
{
	gtk_widget_destroy (dialog);
	gtk_main_quit ();
}

/*
 * set_icon - helper function to bind an icon from a given GnomeIconTheme to
 * a given GtkImage.
 */
static void
set_icon (GtkImage *image, GtkIconTheme *theme, const char *name, const char *fallback)
{
	GdkPixbuf *pixbuf;
	struct stat st;
	
retry:
	
	if (*name == '/') {
		if (stat (name, &st) == 0 && S_ISREG (st.st_mode)) {
			gtk_image_set_from_file (image, name);
			return;
		} else if (fallback && name != fallback) {
			name = fallback;
			goto retry;
		} else {
			name = "image-missing";
		}
	}
	
	if ((pixbuf = gtk_icon_theme_load_icon (theme, name, 48, 0, NULL))) {
		gtk_image_set_from_pixbuf (image, pixbuf);
		g_object_unref (pixbuf);
	} else if (fallback && name != fallback) {
		name = fallback;
		goto retry;
	}
}

static struct {
	const char *name;
	const char *icon;
	const char *fallback;
} icons[] = {
	{ "digital_camera_image",   "camera-photo",          NULL             },
	{ "keyboard_image",         "input-keyboard",        NULL             },
	{ "mouse_image",            "input-mouse",           NULL             },
	{ "palm_image",             "pda-palm",              "palm-pilot"     },
	{ "pocketpc_image",         "pda-pocketpc",          "palm-pilot"     },
	{ "printer_image",          "printer",               NULL             },
	{ "scanner_image",          "scanner",               NULL             },
	{ "tablet_image",           "input-tablet",          NULL             },
	{ "videocam_image",         "camera-video",          NULL             },
	{ "webcam_image",           "camera-web",            NULL             }
};

static void
theme_changed_cb (GtkIconTheme *theme, GladeXML *xml)
{
	GtkWidget *icon;
	size_t i;
	
	for (i = 0; i < G_N_ELEMENTS (icons); i++) {
		icon = glade_xml_get_widget (xml, icons[i].name);
		set_icon (GTK_IMAGE (icon), theme, icons[i].icon, icons[i].fallback);
	}
}

static void
show_props (void)
{
	GtkIconTheme *theme;
	char *filename;
	size_t i;
	
	filename = g_concat_dir_and_file (GLADEDIR, GLADE_XML_FILE);
	xml = glade_xml_new (filename, NULL, PACKAGE);
	g_free (filename);
	
	if (xml == NULL) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						 _("Could not load the main interface"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("Please make sure that the "
							    "volume manager is properly "
							    "installed"));
		
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		exit (EXIT_FAILURE);
	}
	
	props = glade_xml_get_widget (xml, "dialog1");
	g_signal_connect (props, "response", G_CALLBACK (close_cb), NULL);
	
	theme = gtk_icon_theme_get_default ();
	g_signal_connect (theme, "changed", G_CALLBACK (theme_changed_cb), xml);
	theme_changed_cb (theme, xml);
	
	gtk_window_set_default_icon_name ("gnome-dev-cdrom");
	
	gvm_settings_hash = g_hash_table_new (g_str_hash, g_str_equal);
	for (i = 0; i < G_N_ELEMENTS (gvm_settings); i++) {
		const char *name;
		
		g_hash_table_insert (gvm_settings_hash, gvm_settings[i].key, GINT_TO_POINTER (i + 1));
		
		name = strrchr (gvm_settings[i].key, '/') + 1;
		gvm_settings[i].widget = glade_xml_get_widget (xml, name);
		if (gvm_settings[i].type == TYPE_STRING) {
			GtkWidget *fileentry;
			char *str;
			
			str = gconf_client_get_string (gconf, gvm_settings[i].key, NULL);
			gtk_entry_set_text (GTK_ENTRY (gvm_settings[i].widget), str ? str : "");
			g_free (str);
			
			str = g_strdup_printf ("%s_fileentry", name);
			fileentry = gnome_file_entry_gnome_entry (GNOME_FILE_ENTRY (glade_xml_get_widget (xml, str)));
			g_free (str);
			
			/* FIXME: do we really want to share the same history_id across CDR, CDA, and DVD command file entries? */
			gnome_entry_set_history_id (GNOME_ENTRY (fileentry), "CD_CAPPLET_ID");
			gtk_combo_set_case_sensitive (GTK_COMBO (fileentry), FALSE);
			
			g_signal_connect (gvm_settings[i].widget, "changed", G_CALLBACK (changed_cb), (void *) gvm_settings[i].key);
		} else if (gvm_settings[i].type == TYPE_BOOL) {
			gboolean bool;
			
			bool = gconf_client_get_bool (gconf, gvm_settings[i].key, NULL);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gvm_settings[i].widget), bool);
			g_signal_connect (gvm_settings[i].widget, "toggled", G_CALLBACK (toggled_cb), (void *) gvm_settings[i].key);
		} else if (gvm_settings[i].type == TYPE_COMBO) {
			GtkListStore *model;
			GtkTreeIter iter;
			GtkCellRenderer *cell;
			char *str;

			model = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter,
					    0, "umount",
					    1, _("Umount device"),
					    -1);
			gtk_list_store_append (model, &iter);
			gtk_list_store_set (model, &iter,
					    0, "open",
					    1, _("Browse in filemanager"),
					    -1);

			gtk_combo_box_set_model (GTK_COMBO_BOX (gvm_settings[i].widget), GTK_TREE_MODEL (model));

			cell = gtk_cell_renderer_text_new ();
			gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(gvm_settings[i].widget), cell, FALSE);
			gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(gvm_settings[i].widget), cell, "text", 1);

			str = gconf_client_get_string (gconf, gvm_settings[i].key, NULL);
			if (strcmp(str, "umount") == 0) {
				gtk_combo_box_set_active (GTK_COMBO_BOX (gvm_settings[i].widget), 0);
			} else {
				gtk_combo_box_set_active (GTK_COMBO_BOX (gvm_settings[i].widget), 1);
			}

			g_signal_connect (gvm_settings[i].widget, "changed", G_CALLBACK (changed_combo_cb), (void *) gvm_settings[i].key);
			g_free (str);
		} else if (gvm_settings[i].type == TYPE_SPIN) {
			int value;

			value = gconf_client_get_int (gconf, gvm_settings[i].key, NULL);
			gtk_spin_button_set_value (GTK_SPIN_BUTTON(gvm_settings[i].widget), value);
			g_signal_connect (gvm_settings[i].widget, "value-changed", G_CALLBACK (change_value_cb), (void *) gvm_settings[i].key);
		} else {
			g_assert_not_reached ();
		}
	}
	
	set_sensitivity ();
	
	gtk_widget_show_all (props);
}

/** Check if HAL is running
 *
 * @return			TRUE if HAL is running and working.
 *				FALSE otherwise.
 */
static gboolean
check_if_hal_is_running (void)
{
	LibHalContext *ctx;
	char **devices;
	int nr;
	DBusError error;
	DBusConnection *conn;
	
	if (!(ctx = libhal_ctx_new ()))
		return FALSE;
	
	dbus_error_init (&error);
	
	conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	if (dbus_error_is_set (&error)) {
		dbus_error_free (&error);
		return FALSE;
	}
	
	libhal_ctx_set_dbus_connection (ctx, conn);
	
	if (!libhal_ctx_init (ctx, &error)) {
		dbus_error_free (&error);
		return FALSE;
	}
	
	/*
	 * Do something to ping the HAL daemon - the above functions will
	 * succeed even if hald is not running, so long as DBUS is.  But we
	 * want to exit silently if hald is not running, to behave on
	 * pre-2.6 systems.
	 */
	if (!(devices = libhal_get_all_devices (ctx, &nr, &error))) {
		dbus_error_free (&error);
		
		libhal_ctx_shutdown (ctx, NULL);
		libhal_ctx_free (ctx);
		
		return FALSE;
	}
	
	libhal_free_string_array (devices);
	
	libhal_ctx_shutdown (ctx, NULL);
	libhal_ctx_free (ctx);
	
	return TRUE;
}

/*
 * check_clipboard - check if the CLIPBOARD_NAME clipboard is available
 *
 * Returns TRUE if the CLIPBOARD_NAME clipboard is out there and FALSE
 * otherwise
 */
static gboolean
check_clipboard (void)
{
	Atom clipboard_atom = gdk_x11_get_xatom_by_name (CLIPBOARD_NAME);

	return XGetSelectionOwner (GDK_DISPLAY (), clipboard_atom) != None;
}

/*
 * check_daemon - check if the daemon itself is running. if not, and if we
 * have configuration options set that would make it worthwhile to run, run
 * it.
 */
static void
check_daemon (void)
{
	gboolean need_daemon = FALSE;
	GError *error = NULL;
	char *argv[2];
	size_t i;
	
	for (i = 0; !need_daemon && i < G_N_ELEMENTS (gvm_settings); i++) {
		if (gvm_settings[i].need_daemon)
			need_daemon = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gvm_settings[i].widget));
	}
	
	if (!need_daemon || check_clipboard ()) 
		return;
	
	argv[0] = LIBEXECDIR "/gnome-volume-manager";
	argv[1] = NULL;
	g_spawn_async (g_get_home_dir (), argv, NULL, 0, NULL,
			NULL, NULL, &error);
	
	if (error) {
		GtkWidget *message = gtk_message_dialog_new
			(GTK_WINDOW (props), GTK_DIALOG_DESTROY_WITH_PARENT,
			 GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			 _("Error starting gnome-volume-manager daemon:\n%s"),
			 error->message);
		
		gtk_dialog_run (GTK_DIALOG (message));
		
		g_error_free (error);
		
		exit (EXIT_FAILURE);
	}
}


int
main (int argc, char **argv)
{
	gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_NONE, GNOME_PARAM_NONE);
	
	glade_gnome_init ();
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
	
	/* bail out now if hald is not even running */
	if (!check_if_hal_is_running ()) {
		GtkWidget *dialog;
		
		dialog = gtk_message_dialog_new (NULL,
						 0, GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_OK,
						 _("Volume management not supported"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("The \"hald\" service is required but not currently "
							    "running. Enable the service and rerun this application, "
							    "or contact your system administrator."));
		
		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);
		exit (EXIT_FAILURE);
	}
	
	gconf = gconf_client_get_default ();
	gconf_client_add_dir (gconf, GCONF_ROOT_SANS_SLASH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);
	gconf_client_notify_add (gconf, GCONF_ROOT_SANS_SLASH,
				 gconf_changed_cb, NULL, NULL, NULL);
	
	show_props ();
	check_daemon ();
	
	gtk_main ();
	
	g_hash_table_destroy (gvm_settings_hash);
	g_object_unref (gconf);
	g_object_unref (xml);
	
	return 0;
}
