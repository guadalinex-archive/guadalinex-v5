/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * src/manager.c - GNOME Volume Manager
 *
 * Jeffrey Stedfast <fejj@novell.com>
 * Robert Love <rml@novell.com>
 *
 * gnome-volume-manager is a simple policy engine that implements a state
 * machine in response to events from HAL.  Responding to these events,
 * gnome-volume-manager implements automount, autorun, autoplay, automatic
 * photo management, and so on.
 *
 * Licensed under the GNU GPL v2.  See COPYING.
 *
 * (C) Copyright 2005-2007 Novell, Inc.
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <utmp.h>

#include <locale.h>

#ifdef HAVE_CODESET
#include <langinfo.h>
#endif

#ifdef ENABLE_NOTIFY
#include <sys/statvfs.h>
#include <libnotify/notify.h>
#endif

#include <gnome.h>
#include <glib/gi18n.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libhal.h>

#include "gvm.h"

#ifdef GVM_DEBUG
# define dbg(fmt,arg...) fprintf(stderr, "%s/%d: " fmt,__FILE__,__LINE__,##arg)
#else
# define dbg(fmt,arg...) do { } while(0)
#endif

#define warn(fmt,arg...) g_warning("%s/%d: " fmt,__FILE__,__LINE__,##arg)

#define NAUTILUS_COMMAND "nautilus -n --no-desktop %m"

static struct gvm_configuration config;
static DBusConnection *dbus_connection = NULL;
static char *gnome_mount = NULL;
static LibHalContext *hal_ctx;

struct _MountPolicy {
	char *udi;
	int apply;
};

/* Table of UDI's for volumes being mounted by g-v-m that we need to apply policy to */
static GHashTable *mount_table = NULL;
static GHashTable *device_table = NULL;

/* List of UDI's of all volumes mounted during the lifetime of the program */
static GSList *mounted_volumes = NULL;

/* Table of active dialogs indexed by the udi of the device in question */
static GHashTable *dialogs = NULL;

/* List of UDIs of all mounted volumes being watched for low-disk notification */
static GHashTable *statfs_mounts = NULL;

/* Timeout for the callback to do statfs () on mounted volumes */
static guint statfs_id = 0;


typedef struct {
        gboolean notified;
        double last_notified;
	char *udi;
} statfs_mount_info;

#ifdef ENABLE_NOTIFY
static void statfs_mount_info_add (const char *udi);
static void statfs_mount_info_remove (const char *udi);
static void statfs_mount_info_free (statfs_mount_info *info);
static gboolean gvm_statfs_check_space (const char *udi, statfs_mount_info *info, gpointer user_data);
#endif

static gboolean gvm_dbus_init (void);
static LibHalContext *gvm_hal_init (void);

static gboolean gvm_user_is_local (void);
static gboolean gvm_user_is_active (void);


typedef enum {
	TYPE_BOOL,
	TYPE_STRING,
	TYPE_FLOAT
} type_t;

enum {
	FILEMANAGER,
	AUTOBROWSE,
	AUTOBURN,
	AUTOBURN_AUDIO_CD_COMMAND,
	AUTOBURN_DATA_CD_COMMAND,
	AUTOIPOD,
	AUTOIPOD_COMMAND,
	AUTOKEYBOARD,
	AUTOKEYBOARD_COMMAND,
	AUTOMOUNT_DRIVES,
	AUTOMOUNT_MEDIA,
	AUTOOPEN,
	AUTOOPEN_PATH,
	AUTOPHOTO,
	AUTOPHOTO_COMMAND,
	AUTOPILOT,
	AUTOPILOT_COMMAND,
	AUTOPLAY_CDA,
	AUTOPLAY_CDA_COMMAND,
	AUTOPLAY_DVD,
	AUTOPLAY_DVD_COMMAND,
	AUTOPLAY_VCD,
	AUTOPLAY_VCD_COMMAND,
	AUTOPOCKETPC,
	AUTOPOCKETPC_COMMAND,
	AUTOPRINTER,
	AUTOPRINTER_COMMAND,
	AUTORUN,
	AUTORUN_PATH,
	AUTOSCANNER,
	AUTOSCANNER_COMMAND,
	AUTOTABLET,
	AUTOTABLET_COMMAND,
	AUTOWEBCAM,
	AUTOWEBCAM_COMMAND,
	PERCENT_THRESHOLD,
	PERCENT_FREED,
	PERCENT_USED
};

static struct {
	char *key;
	type_t type;
	void *var;
} gvm_settings[] = {
	{ GCONF_ROOT "filemanager",               TYPE_STRING, &config.filemanager               },
	{ GCONF_ROOT "autobrowse",                TYPE_BOOL,   &config.autobrowse                },
	{ GCONF_ROOT "autoburn",                  TYPE_BOOL,   &config.autoburn                  },
	{ GCONF_ROOT "autoburn_audio_cd_command", TYPE_STRING, &config.autoburn_audio_cd_command },
	{ GCONF_ROOT "autoburn_data_cd_command",  TYPE_STRING, &config.autoburn_data_cd_command  },
	{ GCONF_ROOT "autoipod",                  TYPE_BOOL,   &config.autoipod                  },
	{ GCONF_ROOT "autoipod_command",          TYPE_STRING, &config.autoipod_command          },
	{ GCONF_ROOT "autokeyboard",              TYPE_BOOL,   &config.autokeyboard              },
	{ GCONF_ROOT "autokeyboard_command",      TYPE_STRING, &config.autokeyboard_command      },
	{ GCONF_ROOT "automount_drives",          TYPE_BOOL,   &config.automount_drives          },
	{ GCONF_ROOT "automount_media",           TYPE_BOOL,   &config.automount_media           },
	{ GCONF_ROOT "automouse",                 TYPE_BOOL,   &config.automouse                 },
	{ GCONF_ROOT "automouse_command",         TYPE_STRING, &config.automouse_command         },
	{ GCONF_ROOT "autoopen",                  TYPE_BOOL,   &config.autoopen                  },
	{ GCONF_ROOT "autoopen_path",             TYPE_STRING, &config.autoopen_path             },
	{ GCONF_ROOT "autophoto",                 TYPE_BOOL,   &config.autophoto                 },
	{ GCONF_ROOT "autophoto_command",         TYPE_STRING, &config.autophoto_command         },
	{ GCONF_ROOT "autopalmsync",              TYPE_BOOL,   &config.autopilot                 },
	{ GCONF_ROOT "autopalmsync_command",      TYPE_STRING, &config.autopilot_command         },
	{ GCONF_ROOT "autoplay_cda",              TYPE_BOOL,   &config.autoplay_cda              },
	{ GCONF_ROOT "autoplay_cda_command",      TYPE_STRING, &config.autoplay_cda_command      },
	{ GCONF_ROOT "autoplay_dvd",              TYPE_BOOL,   &config.autoplay_dvd              },
	{ GCONF_ROOT "autoplay_dvd_command",      TYPE_STRING, &config.autoplay_dvd_command      },
	{ GCONF_ROOT "autoplay_vcd",              TYPE_BOOL,   &config.autoplay_vcd              },
	{ GCONF_ROOT "autoplay_vcd_command",      TYPE_STRING, &config.autoplay_vcd_command      },
	{ GCONF_ROOT "autopocketpc",              TYPE_BOOL,   &config.autopocketpc              },
	{ GCONF_ROOT "autopocketpc_command",      TYPE_STRING, &config.autopocketpc_command      },
	{ GCONF_ROOT "autoprinter",               TYPE_BOOL,   &config.autoprinter               },
	{ GCONF_ROOT "autoprinter_command",       TYPE_STRING, &config.autoprinter_command       },
	{ GCONF_ROOT "autorun",                   TYPE_BOOL,   &config.autorun                   },
	{ GCONF_ROOT "autorun_path",              TYPE_STRING, &config.autorun_path              },
	{ GCONF_ROOT "autoscanner",               TYPE_BOOL,   &config.autoscanner               },
	{ GCONF_ROOT "autoscanner_command",       TYPE_STRING, &config.autoscanner_command       },
	{ GCONF_ROOT "autotablet",                TYPE_BOOL,   &config.autotablet                },
	{ GCONF_ROOT "autotablet_command",        TYPE_STRING, &config.autotablet_command        },
	{ GCONF_ROOT "autowebcam",                TYPE_BOOL,   &config.autowebcam                },
	{ GCONF_ROOT "autowebcam_command",        TYPE_STRING, &config.autowebcam_command        },
	{ GCONF_ROOT "percent_threshold",         TYPE_FLOAT,  &config.percent_threshold         },
	{ GCONF_ROOT "percent_used",              TYPE_FLOAT,  &config.percent_used              },
};

static GHashTable *gvm_settings_hash = NULL;


struct _GvmPromptButton {
	const char *label;
	const char *stock;
	int response_id;
};

enum {
	GVM_RESPONSE_NONE,
	GVM_RESPONSE_RUN,
	GVM_RESPONSE_OPEN,
	GVM_RESPONSE_PLAY,
	GVM_RESPONSE_BROWSE,
	GVM_RESPONSE_SYNC_MUSIC,
	GVM_RESPONSE_IMPORT_PHOTOS,
	GVM_RESPONSE_WRITE_AUDIO_CD,
	GVM_RESPONSE_WRITE_DATA_CD,
};

static struct _GvmPromptButton GVM_BUTTONS_AUTORUN[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("_Allow Auto-Run"), NULL, GVM_RESPONSE_RUN },
};

static struct _GvmPromptButton GVM_BUTTONS_AUTOOPEN[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("_Open"), NULL, GVM_RESPONSE_OPEN },
};

static struct _GvmPromptButton GVM_BUTTONS_CAMERA[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("Import _Photos"), NULL, GVM_RESPONSE_IMPORT_PHOTOS },
};

static struct _GvmPromptButton GVM_BUTTONS_STORAGE_CAMERA[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("_Open Folder"), NULL, GVM_RESPONSE_BROWSE },
	{ N_("Import _Photos"), NULL, GVM_RESPONSE_IMPORT_PHOTOS },
};

static struct _GvmPromptButton GVM_BUTTONS_IPOD_PHOTO[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("Import _Photos"), NULL, GVM_RESPONSE_IMPORT_PHOTOS },
	{ N_("Manage _Music"), NULL, GVM_RESPONSE_SYNC_MUSIC },
};

static struct _GvmPromptButton GVM_BUTTONS_MIXED_CD[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("_Browse Files"), NULL, GVM_RESPONSE_BROWSE },
	{ N_("_Play CD"), NULL, GVM_RESPONSE_PLAY },
};

static struct _GvmPromptButton GVM_BUTTONS_WRITE_CD[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("Make _Audio CD"), NULL, GVM_RESPONSE_WRITE_AUDIO_CD },
	{ N_("Make _Data CD"), NULL, GVM_RESPONSE_WRITE_DATA_CD },
};

static struct _GvmPromptButton GVM_BUTTONS_WRITE_DVD[] = {
	{ N_("Ig_nore"), NULL, GTK_RESPONSE_CANCEL },
	{ N_("Make _DVD"), NULL, GVM_RESPONSE_WRITE_DATA_CD },
};

typedef enum {
	GVM_PROMPT_AUTORUN,
	GVM_PROMPT_AUTOOPEN,
	GVM_PROMPT_IMPORT_CAMERA,
	GVM_PROMPT_IMPORT_STORAGE_CAMERA,
	GVM_PROMPT_IMPORT_PHOTOS,
	GVM_PROMPT_IPOD_PHOTO,
	GVM_PROMPT_CDA_EXTRA,
	GVM_PROMPT_WRITE_CDR,
	GVM_PROMPT_WRITE_DVD,
} GvmPrompt;

typedef struct _GvmPromptCtx GvmPromptCtx;
typedef void (* GvmPromptCallback) (GvmPromptCtx *ctx, int action);

struct _GvmPromptCtx {
	GvmPrompt prompt;
	
	GvmPromptCallback callback;
	
	char *udi;
	char *device;
	char *mount_point;
	char *path;
};

static struct {
	GtkDialogFlags flags;
	
	const char *icon;
	
	const char *help_uri;
	
	struct _GvmPromptButton *buttons;
	int n_buttons;
	
	int default_response;
	
	const char *title;
	const char *primary;
	const char *secondary;
	int secondary_has_args;
	int secondary_has_mockup;
	
	const char *ask_again_key;
	const char *ask_again_label;
} gvm_prompts[] = {
	{ 0, "gnome-fs-executable", NULL,
	  GVM_BUTTONS_AUTORUN, G_N_ELEMENTS (GVM_BUTTONS_AUTORUN),
	  GVM_RESPONSE_RUN,
	  N_("Auto-Run Confirmation"),
	  N_("Auto-run capability detected."),
	  N_("Would you like to allow <b>'${0}'</b> to run?"), TRUE, TRUE,
	  NULL, NULL },
	{ 0, "gnome-fs-executable", NULL,
	  GVM_BUTTONS_AUTOOPEN, G_N_ELEMENTS (GVM_BUTTONS_AUTOOPEN),
	  GVM_RESPONSE_RUN,
	  N_("Auto-Open Confirmation"),
	  N_("Auto-Open capability detected."),
	  N_("Would you like to open <b>'${0}'</b>?"), TRUE, TRUE,
	  NULL, NULL },
	{ 0, "camera-photo", NULL,
	  GVM_BUTTONS_CAMERA, G_N_ELEMENTS (GVM_BUTTONS_CAMERA),
	  GVM_RESPONSE_IMPORT_PHOTOS,
	  N_("Camera Import"),
	  N_("A camera has been detected."),
	  N_("There are photos on the camera. Would you like to add these pictures to your album?"), FALSE, FALSE,
	  GCONF_ROOT "prompts/camera_import_photos", N_("_Always perform this action") },
	{ 0, "camera-photo", NULL,
	  GVM_BUTTONS_STORAGE_CAMERA, G_N_ELEMENTS (GVM_BUTTONS_STORAGE_CAMERA),
	  GVM_RESPONSE_IMPORT_PHOTOS,
	  N_("Camera Import"),
	  N_("A camera has been detected."),
	  N_("There are photos on the camera. Would you like to add these pictures to your album?"), FALSE, FALSE,
	  GCONF_ROOT "prompts/storage_camera_import_photos", N_("_Always perform this action") },
	{ 0, "camera-photo", NULL,
	  GVM_BUTTONS_CAMERA, G_N_ELEMENTS (GVM_BUTTONS_CAMERA),
	  GVM_RESPONSE_IMPORT_PHOTOS,
	  N_("Photo Import"),
	  N_("A photo card has been detected."),
	  N_("There are photos on the card. Would you like to add these pictures to your album?"), FALSE, FALSE,
	  GCONF_ROOT "prompts/device_import_photos", N_("_Always perform this action") },
	{ 0, "gnome-dev-ipod", NULL,
	  GVM_BUTTONS_IPOD_PHOTO, G_N_ELEMENTS (GVM_BUTTONS_IPOD_PHOTO),
	  GVM_RESPONSE_SYNC_MUSIC,
	  N_("Photos and Music"),
	  N_("Photos were found on your music device."),
	  N_("Would you like to import the photos or manage its music?"), FALSE, FALSE,
	  GCONF_ROOT "prompts/ipod_photo", N_("_Always perform this action") },
	{ 0, "gnome-dev-cdrom-audio", NULL,
	  GVM_BUTTONS_MIXED_CD, G_N_ELEMENTS (GVM_BUTTONS_MIXED_CD),
	  GVM_RESPONSE_PLAY,
	  N_("Mixed Audio and Data CD"),
	  N_("The CD in the drive contains both music and files."),
	  N_("Would you like to listen to music or browse the files?"), FALSE, FALSE,
	  GCONF_ROOT "prompts/cd_mixed", N_("_Always perform this action") },
	{ 0, "gnome-dev-cdrom", NULL,
	  GVM_BUTTONS_WRITE_CD, G_N_ELEMENTS (GVM_BUTTONS_WRITE_CD),
	  GVM_RESPONSE_WRITE_DATA_CD,
	  N_("Blank CD Inserted"),
	  N_("You have inserted a blank disc."),
	  N_("What would you like to do?"), FALSE, FALSE,
	  NULL, NULL },
	{ 0, "gnome-dev-disc-dvdr", NULL,
	  GVM_BUTTONS_WRITE_DVD, G_N_ELEMENTS (GVM_BUTTONS_WRITE_DVD),
	  GVM_RESPONSE_WRITE_DATA_CD,
	  N_("Blank DVD Inserted"),
	  N_("You have inserted a blank disc."),
	  N_("What would you like to do?"), FALSE, FALSE,
	  NULL, NULL },
};


static GvmPromptCtx *
gvm_prompt_ctx_new (GvmPrompt prompt, GvmPromptCallback callback, const char *udi,
		    const char *device, const char *mount_point, const char *path)
{
	GvmPromptCtx *ctx;
	
	ctx = g_malloc (sizeof (GvmPromptCtx));
	ctx->prompt = prompt;
	ctx->callback = callback;
	ctx->udi = g_strdup (udi);
	ctx->device = g_strdup (device);
	ctx->mount_point = g_strdup (mount_point);
	ctx->path = g_strdup (path);
	
	return ctx;
}

static void
gvm_prompt_ctx_free (GvmPromptCtx *ctx)
{
	g_free (ctx->udi);
	g_free (ctx->device);
	g_free (ctx->mount_point);
	g_free (ctx->path);
	g_free (ctx);
}

static void
prompt_response_cb (GtkWidget *dialog, int response, GvmPromptCtx *ctx)
{
	GtkToggleButton *checkbox;
	GConfClient *gconf;
	GError *err = NULL;
	
	if (response == GTK_RESPONSE_HELP) {
		g_signal_stop_emission_by_name (dialog, "response");
		gnome_url_show (gvm_prompts[ctx->prompt].help_uri, &err);
		if (err) {
			warn ("Unable to run help uri: %s", err->message);
			g_error_free (err);
		}
		
		return;
	}
	
	checkbox = g_object_get_data ((GObject *) dialog, "checkbox");
	if (checkbox && gtk_toggle_button_get_active (checkbox) && response != GTK_RESPONSE_CANCEL) {
		gconf = gconf_client_get_default ();
		gconf_client_set_int (gconf, gvm_prompts[ctx->prompt].ask_again_key, response, NULL);
		g_object_unref (gconf);
	}
	
	g_hash_table_remove (dialogs, ctx->udi);
	ctx->callback (ctx, response);
	gtk_widget_destroy (dialog);
	gvm_prompt_ctx_free (ctx);
}

static char *
argv_expand (const char *format, int argc, char **argv)
{
	const char *start, *inptr;
	GString *string;
	char *str;
	int i;
	
	string = g_string_new ("");
	start = inptr = format;
	
	while (*inptr) {
		while (*inptr) {
			if (inptr[0] == '$' && inptr[1] == '{' && inptr[2] >= '0' && inptr[2] <= '9')
				break;
			inptr++;
		}
		
		if (*inptr == '\0')
			break;
		
		g_string_append_len (string, start, inptr - start);
		
		start = inptr;
		inptr += 2;
		i = strtol (inptr, &str, 10);
		if (*str == '}' && i < argc) {
			start = inptr = str + 1;
			g_string_append (string, argv[i]);
		}
	}
	
	g_string_append (string, start);
	
	str = string->str;
	g_string_free (string, FALSE);
	
	return str;
}

static void
gvm_prompt (GvmPromptCtx *ctx, int argc, char **argv)
{
	GtkWidget *dialog, *hbox, *vbox, *image, *label, *check = NULL;
	GvmPrompt prompt = ctx->prompt;
	GConfClient *gconf;
	GError *err = NULL;
	const char *text;
	int response, i;
	char *buf;
	
	gconf = gconf_client_get_default ();
	
	/* don't prompt the user again if she's already chosen a default action and has asked to not be prompted again */
	if (gvm_prompts[prompt].ask_again_key) {
		response = gconf_client_get_int (gconf, gvm_prompts[prompt].ask_again_key, &err);
		if (response > GVM_RESPONSE_NONE && err == NULL) {
			ctx->callback (ctx, response);
			return;
		}
		
		if (err != NULL)
			g_error_free (err);
	}
	
	dialog = gtk_dialog_new ();
	gtk_widget_ensure_style (dialog);
	gtk_dialog_set_has_separator ((GtkDialog *) dialog, FALSE);
	
	gtk_container_set_border_width ((GtkContainer *) ((GtkDialog *) dialog)->vbox, 0);
	gtk_container_set_border_width ((GtkContainer *) ((GtkDialog *) dialog)->action_area, 12);
	
	if (gvm_prompts[prompt].title)
		gtk_window_set_title ((GtkWindow *) dialog, _(gvm_prompts[prompt].title));
	
	if (gvm_prompts[prompt].flags & GTK_DIALOG_MODAL)
		gtk_window_set_modal ((GtkWindow *) dialog, TRUE);
	
	if (gvm_prompts[prompt].help_uri)
		gtk_dialog_add_button ((GtkDialog *) dialog, GTK_STOCK_HELP, GTK_RESPONSE_HELP);
	
	if (gvm_prompts[prompt].buttons) {
		for (i = 0; i < gvm_prompts[prompt].n_buttons; i++) {
			const char *name;
			
			name = gvm_prompts[prompt].buttons[i].stock ?
				gvm_prompts[prompt].buttons[i].stock :
				_(gvm_prompts[prompt].buttons[i].label);
			
			gtk_dialog_add_button ((GtkDialog *) dialog, name, gvm_prompts[prompt].buttons[i].response_id);
		}
		
		if (gvm_prompts[prompt].default_response != GVM_RESPONSE_NONE)
			gtk_dialog_set_default_response ((GtkDialog *) dialog, gvm_prompts[prompt].default_response);
	} else {
		gtk_dialog_add_button ((GtkDialog *) dialog, GTK_STOCK_OK, GTK_RESPONSE_OK);
	}
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width ((GtkContainer *) hbox, 12);
	
	/* set the icon */
	image = gtk_image_new_from_icon_name (gvm_prompts[prompt].icon, GTK_ICON_SIZE_DIALOG);
	
	gtk_misc_set_alignment ((GtkMisc *) image, 0.0, 0.0);
	gtk_box_pack_start ((GtkBox *) hbox, image, FALSE, FALSE, 12);
	gtk_widget_show (image);
	
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start ((GtkBox *) hbox, vbox, FALSE, FALSE, 0);
	
	/* build the primary text */
	buf = g_strdup_printf ("<span weight=\"bold\" size=\"larger\">%s</span>", _(gvm_prompts[prompt].primary));
	
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
	gtk_label_set_line_wrap ((GtkLabel *) label, FALSE);
	gtk_label_set_markup ((GtkLabel *) label, buf);
	g_free (buf);
	
	gtk_box_pack_start ((GtkBox *) vbox, label, FALSE, FALSE, 0);
	gtk_widget_show (label);
	
	/* build the secondary text */
	buf = NULL;
	if (gvm_prompts[prompt].secondary_has_args) {
		text = buf = argv_expand (_(gvm_prompts[prompt].secondary), argc, argv);
	} else {
		text = _(gvm_prompts[prompt].secondary);
	}
	
	label = gtk_label_new (NULL);
	gtk_misc_set_alignment ((GtkMisc *) label, 0.0, 0.5);
	/*gtk_label_set_selectable ((GtkLabel *) label, TRUE);*/
	gtk_label_set_line_wrap ((GtkLabel *) label, TRUE);
	if (gvm_prompts[prompt].secondary_has_mockup)
		gtk_label_set_markup ((GtkLabel *) label, text);
	else
		gtk_label_set_text ((GtkLabel *) label, text);
	g_free (buf);
	
	gtk_box_pack_start ((GtkBox *) vbox, label, FALSE, FALSE, 6);
	gtk_widget_show (label);
	
	gtk_box_pack_start ((GtkBox *) ((GtkDialog *) dialog)->vbox, hbox, FALSE, FALSE, 0);
	
	/* conditionally add a checkbox to never bother the user again */
	if (gvm_prompts[prompt].ask_again_key && gvm_prompts[prompt].ask_again_label) {
		check = gtk_check_button_new_with_mnemonic (_(gvm_prompts[prompt].ask_again_label));
		gtk_container_set_border_width ((GtkContainer *) check, 0);
		gtk_box_pack_start ((GtkBox *) vbox, check, FALSE, FALSE, 0);
		g_object_set_data ((GObject *) dialog, "checkbox", check);
		gtk_widget_show (check);
	}
	
	gtk_widget_show (vbox);
	gtk_widget_show (hbox);
	
	g_object_unref (gconf);
	
	g_hash_table_insert (dialogs, ctx->udi, dialog);
	
	g_signal_connect (dialog, "response", G_CALLBACK (prompt_response_cb), ctx);
	
	gtk_widget_show (dialog);
}

static void
to_be_or_not_to_be (void)
{
	/*
	 * If all of the options that control our policy are disabled, then we
	 * have no point in living.  Save the user some memory and exit.
	 */
	/* you used to say live and let live... */
	gboolean live = FALSE;
	size_t i;
	
	/* ...but in this ever changing world in which we live in... */
	for (i = 0; i < G_N_ELEMENTS (gvm_settings) && !live; i++) {
		if (gvm_settings[i].type == TYPE_BOOL)
			live = *((int *) gvm_settings[i].var);
	}
	
	/* makes you give it a cry... */
	if (!live) {
		dbg ("daemon exit: live and let die\n");
		exit (EXIT_SUCCESS);
	}
}

static gboolean
filter_out_media_handling (gint key, gboolean value)
{
       switch (key) {
       case AUTOBROWSE:
       case AUTOBURN:
       case AUTOIPOD:
       case AUTOMOUNT_DRIVES:
       case AUTOMOUNT_MEDIA:
       case AUTOOPEN:
       case AUTOPLAY_CDA:
       case AUTOPLAY_DVD:
       case AUTOPLAY_VCD:
       case AUTORUN:
               return FALSE;
       default:
               return value;
       }
}


/*
 * gvm_load_config - synchronize gconf => config structure
 */
static void
gvm_load_config (void)
{
	size_t i;
	
	gvm_settings_hash = g_hash_table_new (g_str_hash, g_str_equal);
	
	for (i = 0; i < G_N_ELEMENTS (gvm_settings); i++) {
		g_hash_table_insert (gvm_settings_hash, gvm_settings[i].key, GINT_TO_POINTER (i + 1));
		if (gvm_settings[i].type == TYPE_STRING) {
			*((char **) gvm_settings[i].var) =
				gconf_client_get_string (config.client, gvm_settings[i].key, NULL);
			dbg ("setting[%d]: string: %s = %s\n", i, strrchr (gvm_settings[i].key, '/') + 1,
			     *((char **) gvm_settings[i].var) ? *((char **) gvm_settings[i].var): "NULL");
		} else if (gvm_settings[i].type == TYPE_BOOL) {
			*((int *) gvm_settings[i].var) =
				filter_out_media_handling (i, gconf_client_get_bool (config.client, gvm_settings[i].key, NULL));
			dbg ("setting[%d]: bool: %s = %d\n", i, strrchr (gvm_settings[i].key, '/') + 1,
			     *((int *) gvm_settings[i].var));
		} else if (gvm_settings[i].type == TYPE_FLOAT) {
			*((double *) gvm_settings[i].var) =
				gconf_client_get_float (config.client, gvm_settings[i].key, NULL);
			if (*((double *) gvm_settings[i].var) >= 1.0)
				*((double *) gvm_settings[i].var) = 1.0;
			else if (*((double *) gvm_settings[i].var) <= 0.0)
				*((double *) gvm_settings[i].var) = 0.0;
			dbg ("settings[%d]: float: %s = %f\n", i, strrchr (gvm_settings[i].key, '/') + 1,
			     *((double *) gvm_settings[i].var));
		} else {
			g_assert_not_reached ();
		}
	}
	
	to_be_or_not_to_be ();
}

/*
 * gvm_config_changed - gconf_client_notify_add () call back to reload config
 */
static void
gvm_config_changed (GConfClient *client GNUC_UNUSED,
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
	
	if (gvm_settings[which].type == TYPE_STRING) {
		g_free (*((char **) gvm_settings[which].var));
		*((char **) gvm_settings[which].var) = g_strdup (gconf_value_get_string (value));
		dbg ("setting changed: string: %s = %s\n", strrchr (gvm_settings[which].key, '/') + 1,
		     *((char **) gvm_settings[which].var));
	} else if (gvm_settings[which].type == TYPE_BOOL) {
		*((int *) gvm_settings[which].var) = filter_out_media_handling (which, gconf_value_get_bool (value));
		dbg ("setting changed: bool: %s = %d\n", strrchr (gvm_settings[which].key, '/') + 1,
		     *((int *) gvm_settings[which].var));
	} else if (gvm_settings[which].type == TYPE_FLOAT) {
		*((double *) gvm_settings[which].var) = gconf_client_get_float (config.client, gvm_settings[which].key, NULL);
		if (*((double *) gvm_settings[which].var) >= 1.0)
			*((double *) gvm_settings[which].var) = 1.0;
		else if (*((double *) gvm_settings[which].var) <= 0.0)
			*((double *) gvm_settings[which].var) = 0.0;
		dbg ("settings[%d]: float: %s = %f\n", which, strrchr (gvm_settings[which].key, '/') + 1,
		     *((double *) gvm_settings[which].var));
	} else {
		g_assert_not_reached ();
	}
	
	to_be_or_not_to_be ();
}

/*
 * gvm_init_config - initialize gconf client and load config data
 */
static void
gvm_init_config (void)
{
	config.client = gconf_client_get_default ();

	gconf_client_add_dir (config.client, GCONF_ROOT_SANS_SLASH,
			      GCONF_CLIENT_PRELOAD_ONELEVEL, NULL);

	gvm_load_config ();

	gconf_client_notify_add (config.client, GCONF_ROOT_SANS_SLASH,
				 gvm_config_changed, NULL, NULL, NULL);
}

/*
 * gvm_run_command - run the given command, replacing %d with the device node,
 * %h with the HAL UDI and %m with the given path.
 *
 * Returns TRUE if successful or FALSE otherwise.
 */
static gboolean
gvm_run_command (const char *command, const char *udi, const char *device, const char *mount_point)
{
	char *path, *mp = NULL, *dev = NULL;
	const char *inptr, *start;
	GError *error = NULL;
	GString *exec;
	char *argv[4];
	
	g_assert (udi != NULL);
	
	exec = g_string_new (NULL);
	
	/* perform s/%d/device/, s/%m/mount_point/ and s/%h/udi/ */
	start = inptr = command;
	while ((inptr = strchr (inptr, '%')) != NULL) {
		g_string_append_len (exec, start, inptr - start);
		inptr++;
		switch (*inptr) {
		case 'd':
			g_string_append (exec, device ? device : "");
			break;
		case 'm':
			if (mount_point == NULL && libhal_device_query_capability (hal_ctx, udi, "volume", NULL)) {
				mp = libhal_device_get_property_string (hal_ctx, udi, "volume.mount_point", NULL);
				mount_point = mp;
			}
			
			if (mount_point) {
				path = g_shell_quote (mount_point);
				g_string_append (exec, path);
				g_free (path);
			} else {
				g_string_append (exec, "\"\"");
			}
			break;
		case 'h':
			g_string_append (exec, udi);
			break;
		case '%':
			g_string_append_c (exec, '%');
			break;
		default:
			g_string_append_c (exec, '%');
			if (*inptr)
				g_string_append_c (exec, *inptr);
			break;
		}
		
		if (*inptr)
			inptr++;
		start = inptr;
	}
	g_string_append (exec, start);
	
	libhal_free_string (mp);
	libhal_free_string (dev);
	
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	argv[2] = exec->str;
	argv[3] = NULL;
	
	dbg ("executing command: %s\n", exec->str);
	if (!g_spawn_async (g_get_home_dir (), argv, NULL, 0, NULL, NULL, NULL, &error)) {
		warn ("failed to exec %s: %s", exec->str, error->message);
		g_string_free (exec, TRUE);
		g_error_free (error);
		return FALSE;
	}
	
	g_string_free (exec, TRUE);
	
	return TRUE;
}

static gboolean
gvm_check_dir (const char *dirname, const char *name, gboolean check_contents)
{
	gboolean exists = FALSE;
	struct dirent *dent;
	char *path = NULL;
	struct stat st;
	DIR *dir;
	
	if (!(dir = opendir (dirname)))
		return FALSE;
	
	while ((dent = readdir (dir))) {
		if (!g_ascii_strcasecmp (dent->d_name, name)) {
			path = g_build_filename (dirname, dent->d_name, NULL);
			if (stat (path, &st) == 0 && S_ISDIR (st.st_mode))
				exists = TRUE;
			break;
		}
	}
	
	closedir (dir);
	
	if (exists && check_contents) {
		exists = FALSE;
		if ((dir = opendir (path))) {
			while ((dent = readdir (dir))) {
				if (!strcmp (dent->d_name, "..")
				    || !strcmp (dent->d_name, "."))
					continue;
				
				exists = TRUE;
				break;
			}
			
			closedir (dir);
		}
	}
	
	g_free (path);
	
	return exists;
}

/*
 * gvm_check_dvd - is this a Video DVD?  If so, do something about it.
 *
 * Returns TRUE if this was a Video DVD and FALSE otherwise.
 */
static gboolean
gvm_check_dvd (const char *udi, const char *device, const char *mount_point)
{
	gboolean is_dvd;
	
	is_dvd = gvm_check_dir (mount_point, "video_ts", FALSE);
	
	if (is_dvd && config.autoplay_dvd)
		gvm_run_command (config.autoplay_dvd_command, udi, device, mount_point);
	
	return is_dvd;
}

/*
 * gvm_check_vcd - is this a Video CD?  If so, do something about it.
 *
 * Returns TRUE if this was a Video CD and FALSE otherwise.
 */
static gboolean
gvm_check_vcd (const char *udi, const char *device, const char *mount_point)
{
	gboolean is_vcd;
	
	is_vcd = gvm_check_dir (mount_point, "vcd", FALSE);
	
	if (is_vcd && config.autoplay_vcd)
		gvm_run_command (config.autoplay_vcd_command, udi, device, mount_point);
	
	return is_vcd;
}

/*
 * gvm_udi_is_storage_camera - checks if the udi is a mass-storage camera device
 *
 * Returns TRUE if the device is a camera or FALSE otherwise.
 */
static gboolean
gvm_udi_is_storage_camera (const char *udi)
{
	char *physical_device = NULL;
	char *storage_device = NULL;
	gboolean is_camera = FALSE;
	DBusError error;
	
	dbus_error_init (&error);
	if (!(storage_device = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error))) {
		warn ("cannot get block.storage_device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return FALSE;
	}
	
	if (!(physical_device = libhal_device_get_property_string (hal_ctx, storage_device, "storage.physical_device", &error))) {
		warn ("cannot get storage.physical_device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		goto out;
	}
	
	if ((is_camera = libhal_device_query_capability (hal_ctx, physical_device, "camera", NULL)))
		dbg ("Camera detected: %s\n", udi);
	
 out:
	
	libhal_free_string (storage_device);
	libhal_free_string (physical_device);
	
	return is_camera;
}

/*
 * gvm_udi_is_camera - checks if the udi is a supported non-mass-storage camera device
 *
 * Returns TRUE if the device is a camera with libgphoto2 support or FALSE otherwise.
 */
static gboolean
gvm_udi_is_camera (const char *udi)
{
	gboolean is_camera = FALSE;
	char *access_method;
	char *driver;
	
#if 0
	/* Note: this query is not necessary atm because our only caller already has this info */
	if (!libhal_device_query_capability (hal_ctx, udi, "camera", NULL))
		return FALSE;
#endif
	
	if (!(access_method = libhal_device_get_property_string (hal_ctx, udi, "camera.access_method", NULL)))
		return FALSE;
	
	if (!strcmp (access_method, "storage")) {
		/* we only want to match non-storage cameras */
		is_camera = FALSE;
		goto done;
	} else if (!strcmp (access_method, "ptp")) {
		/* ptp cameras are supported by libgphoto2 always */
		is_camera = TRUE;
		goto done;
	} else if (!strcmp (access_method, "libgphoto2")) {
		/* user has an old install of the libgphoto2 fdi files */
		warn ("Please update your libgphoto2 package as the fdi files are outdated!\n");
		if ((driver = libhal_device_get_property_string (hal_ctx, udi, "info.linux.driver", NULL))) {
			/* the old fdi files marked everything as access_method="libgphoto2" so check that
			 * this is a non-mass-storage camera */
			if (!strcmp (driver, "usb-storage"))
				is_camera = FALSE;
		} else {
			is_camera = TRUE;
		}
		
		libhal_free_string (driver);
		
		goto done;
	}
	
	dbg ("Non Mass-Storage Camera detected: %s\n", udi);
	
	is_camera = libhal_device_get_property_bool (hal_ctx, udi, "camera.libgphoto2.support", NULL);
	
 done:
	
	libhal_free_string (access_method);
	
	return is_camera;
}

#if 0
/*
 * gvm_udi_is_ipod - checks if the udi is the mountable volume of an iPod
 *
 * Returns TRUE if the device is an iPod or FALSE otherwise.
 */
static gboolean
gvm_udi_is_ipod (const char *udi)
{
	char *storage_device = NULL;
	gboolean is_ipod = FALSE;
	char *product = NULL;
	DBusError error;
	
	dbus_error_init (&error);
	if (!(storage_device = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error))) {
		warn ("cannot get block.storage_device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return FALSE;
	}
	
	if (!(product = libhal_device_get_property_string (hal_ctx, storage_device, "info.product", &error))) {
		warn ("cannot get info.product property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		goto out;
	}
	
	if ((is_ipod = !strcmp (product, "iPod")))
		dbg ("iPod detected: %s\n", udi);
	else
		dbg ("not an iPod: %s\n", udi);
	
 out:
	
	libhal_free_string (storage_device);
	libhal_free_string (product);
	
	return is_ipod;
}
#endif

/*
 * gvm_udi_is_portable_media_player - checks if the udi is the mountable volume of a media player
 *
 * Returns TRUE if the device is a portable media player or FALSE otherwise.
 */
static gboolean
gvm_udi_is_portable_media_player (const char *udi, gboolean *is_ipod)
{
	char *storage_device = NULL;
	gboolean is_player = FALSE;
	char *product = NULL;
	DBusError error;
	
	if (is_ipod)
		*is_ipod = FALSE;
	
	dbus_error_init (&error);
	if (!(storage_device = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error))) {
		warn ("cannot get block.storage_device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return FALSE;
	}
	
	if (is_ipod && (product = libhal_device_get_property_string (hal_ctx, storage_device, "info.product", NULL))) {
		*is_ipod = !strcmp (product, "iPod");
		libhal_free_string (product);
	}
	
	if ((is_player = libhal_device_query_capability (hal_ctx, storage_device, "portable_audio_player", NULL)))
		dbg ("%s detected: %s\n", is_ipod && *is_ipod ? "iPod" : "Generic music player", udi);
	
	libhal_free_string (storage_device);
	
	return is_player;
}

/*
 * gvm_run_camera - launch the camera application
 */
static void
gvm_run_camera (const char *udi, const char *device, const char *mount_point)
{
	if (config.autophoto_command != NULL)
		gvm_run_command (config.autophoto_command, udi, device, mount_point);
}

/*
 * gvm_check_photos - check if this device has a dcim directory
 *
 * Returns TRUE if there were photos on this device, FALSE otherwise
 */
static gboolean
gvm_check_photos (const char *udi, const char *device, const char *mount_point)
{
	DBusError error;
	
	if (!gvm_check_dir (mount_point, "dcim", TRUE))
		return FALSE;
	
	dbg ("Photos detected: %s\n", mount_point);
	
	/* add the "content.photos" capability to this device */
	dbus_error_init (&error);
	if (!libhal_device_add_capability (hal_ctx, udi, "content.photos", &error)) {
		warn ("failed to set content.photos on %s: %s", device, error.message);
		dbus_error_free (&error);
	}
	
	return TRUE;
}

/*
 * gvm_run_portable_media_player - launch the ipod application
 */
static void
gvm_run_portable_media_player (const char *udi, const char *device, const char *mount_point)
{
	if (config.autoipod_command != NULL)
		gvm_run_command (config.autoipod_command, udi, device, mount_point);
}

/*
 * gvm_run_pilot - sync the pda palm pilot
 */
static void
gvm_run_pilot (const char *udi)
{
	DBusError error;
	char *device;
	
	if (config.autopilot_command == NULL)
		return;
	
	dbus_error_init (&error);
	/* pda.palm.hotsync_interface should be the same as serial.device, but just in case... */
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "pda.palm.hotsync_interface", &error))) {
		warn ("cannot get pda.palm.hotsync_interface property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return;
	}
	
	gvm_run_command (config.autopilot_command, udi, device, NULL);
	libhal_free_string (device);
}

/*
 * gvm_run_pocketpc - sync the pda palm pilot
 */
static void
gvm_run_pocketpc (const char *udi)
{
	DBusError error;
	char *device;
	
	if (config.autopocketpc_command == NULL)
		return;
	
	dbus_error_init (&error);
	/* pda.palm.hotsync_interface should be the same as serial.device, but just in case... */
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "pda.pocketpc.hotsync_interface", &error))) {
		warn ("cannot get pda.pocketpc.hotsync_interface property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return;
	}
	
	gvm_run_command (config.autopocketpc_command, udi, device, NULL);
	libhal_free_string (device);
}

/*
 * gvm_run_printer - launch the printer application
 */
static void
gvm_run_printer (const char *udi)
{
	DBusError error;
	char *device;
	
	if (config.autoprinter_command == NULL)
		return;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "printer.device", &error))) {
		warn ("cannot get printer.device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return;
	}
	
	gvm_run_command (config.autoprinter_command, udi, device, NULL);
	libhal_free_string (device);
}

/*
 * gvm_run_scanner - launch the scanner application
 */
static void
gvm_run_scanner (const char *udi)
{
	DBusError error;
	char *device;
	
	if (config.autoscanner_command == NULL)
		return;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "scanner.device", &error))) {
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
                /* check parent's linux.device_file */
                device = libhal_device_get_property_string(hal_ctx, udi, "info.parent", NULL);
                if (device) {
                        device = libhal_device_get_property_string (hal_ctx, device, 
                                "linux.device_file", &error);
                }
                if (!device) {
                        warn ("cannot get scanner.device property: %s", error.message);
                        return;
                }
	}
	
	gvm_run_command (config.autoscanner_command, udi, device, NULL);
	libhal_free_string (device);
}

/*
 * gvm_run_webcam - launch the webcam application
 */
static void
gvm_run_webcam (const char *udi)
{
	DBusError error;
	char *device;
	
	if (config.autowebcam_command == NULL)
		return;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "video4linux.device", &error))) {
		warn ("cannot get video4linux.device property: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		return;
	}
	
	gvm_run_command (config.autowebcam_command, udi, device, NULL);
	libhal_free_string (device);
}

static void
import_photos_cb (GvmPromptCtx *ctx, int action)
{
	if (action == GVM_RESPONSE_IMPORT_PHOTOS)
		gvm_run_camera (ctx->udi, ctx->device, ctx->mount_point);
}

static gboolean
is_exe (const char *path)
{
	size_t len;
	
	return (len = strlen (path)) > 4 && !g_ascii_strcasecmp (path + len - 4, ".exe");
}

static void
gvm_run_filemanager (const char *udi, const char *device, const char *mount_point)
{
	const char *command = NAUTILUS_COMMAND;
	
	if (config.filemanager && *config.filemanager)
		command = config.filemanager;
	
	gvm_run_command (command, udi, device, mount_point);
}

static void
autorun_cb (GvmPromptCtx *ctx, int action)
{
	gboolean autobrowse = TRUE;
	GError *error = NULL;
	const char *prog;
	GPtrArray *args;
	struct stat st;
	char **argv;
	int argc, i;
	
	if (action == GVM_RESPONSE_RUN || action == GVM_RESPONSE_OPEN) {
		if (!g_shell_parse_argv (ctx->path, &argc, &argv, NULL)) {
			argv = g_malloc (sizeof (char *) * 2);
			argv[0] = g_strdup (ctx->path);
			argv[1] = NULL;
			argc = 1;
		}
		
		if (stat (argv[0], &st) == -1) {
			g_strfreev (argv);
			goto autobrowse;
		}
		
		if (S_ISREG (st.st_mode) &&
		    (access (argv[0], R_OK | X_OK) == 0 || is_exe (argv[0]))) {
			/* argv[0] is a program, attempt to run it */
			args = g_ptr_array_new ();
			
			/* If the extension is .exe and the user has wine
			 * installed, try running the application under wine
			 * so that it Just Works(tm) for Grandma. */
			if (is_exe (argv[0]) && (prog = g_find_program_in_path ("wine")))
				g_ptr_array_add (args, (char *) prog);
			
			for (i = 0; i < argc; i++)
				g_ptr_array_add (args, argv[i]);
			g_ptr_array_add (args, NULL);
			
			g_spawn_async (ctx->mount_point, (char **) args->pdata, NULL,
				       0, NULL, NULL, NULL, &error);
			
			g_ptr_array_free (args, TRUE);
			
			if (error)
				warn ("failed to exec %s: %s", ctx->path, error->message);
			else
				autobrowse = FALSE;
		} else if (argc > 1) {
			/* ctx->path is a command-line, but argv[0] is not executable. */
			warn ("failed to exec %s: %s", ctx->path, g_strerror (EACCES));
		} else if ((prog = g_find_program_in_path ("gnome-open"))) {
			/* just open the specified path using gnome-open */
			args = g_ptr_array_new ();
			g_ptr_array_add (args, (char *) prog);
			g_ptr_array_add (args, argv[0]);
			g_ptr_array_add (args, NULL);
			
			g_spawn_async (ctx->mount_point, (char **) args->pdata, NULL,
				       0, NULL, NULL, NULL, &error);
			
			g_ptr_array_free (args, TRUE);
			
			if (error)
				warn ("failed to open %s: %s", ctx->path, error->message);
			else
				autobrowse = FALSE;
		}
		
		g_strfreev (argv);
	}
	
autobrowse:
	
	if (config.autobrowse && autobrowse)
		gvm_run_filemanager (ctx->udi, ctx->device, ctx->mount_point);
}

static char *
canon_path (char *path)
{
	register char *inptr, *outptr;
	
	inptr = outptr = path;
	
	while (*inptr != '\0') {
		if (inptr[0] == '.' && (inptr[1] == '/' || (inptr[1] == '.' && inptr[2] == '/'))) {
			if (inptr[1] == '.') {
				/* ../ */
				inptr += 3;
				
				if (outptr > path) {
					outptr--;
					while (outptr > path && outptr[-1] != '/')
						outptr--;
					if (outptr == path && *outptr == '/')
						outptr++;
				}
			} else {
				/* ./ */
				inptr += 2;
			}
			
			while (*inptr == '/')
				inptr++;
		}
		
		while (*inptr && *inptr != '/')
			*outptr++ = *inptr++;
		
		if (*inptr == '/')
			*outptr++ = *inptr++;
		
		while (*inptr == '/')
			inptr++;
	}
	
	*outptr = '\0';
	
	return path;
}

static char *
unix_path (const char *mount_point, const char *rel_path)
{
	const char *start, *inptr;
	gboolean checkstat = TRUE;
	struct dirent *dent;
	struct stat st;
	GString *path;
	size_t len;
	char *str;
	DIR *dir;
	
	path = g_string_new (mount_point);
	
	inptr = rel_path;
	while (*inptr) {
		start = inptr;
		
		while (*inptr && *inptr != '\\')
			inptr++;
		
		if (checkstat) {
			len = path->len;
			
			g_string_append_c (path, G_DIR_SEPARATOR);
			g_string_append_len (path, start, inptr - start);
			
			if (stat (path->str, &st) == -1) {
				/* find the correct capitalization of the path component */
				g_string_truncate (path, len);
				
				len = inptr - start;
				
				if ((dir = opendir (path->str))) {
					while ((dent = readdir (dir))) {
						if (!g_ascii_strncasecmp (dent->d_name, start, len))
							break;
					}
					
					g_string_append_c (path, G_DIR_SEPARATOR);
					
					if (dent == NULL) {
						/* path component not found */
						g_string_append_len (path, start, len);
						checkstat = FALSE;
					} else
						g_string_append (path, dent->d_name);
					
					closedir (dir);
				} else {
					g_string_append_c (path, G_DIR_SEPARATOR);
					g_string_append_len (path, start, len);
					checkstat = FALSE;
				}
			}
		} else {
			/* some previous path component was not found */
			g_string_append_c (path, G_DIR_SEPARATOR);
			g_string_append_len (path, start, inptr - start);
		}
		
		if (*inptr == '\0')
			break;
		
		inptr++;
	}
	
	str = path->str;
	g_string_free (path, FALSE);
	
	return canon_path (str);
}

static gboolean
readline (FILE *fp, GString *linebuf)
{
	gboolean eoln = FALSE;
	gboolean rv = FALSE;
	char buf[256];
	size_t n;
	
	g_string_truncate (linebuf, 0);
	
	while (!eoln && fgets (buf, sizeof (buf), fp)) {
		n = strlen (buf);
		rv = TRUE;
		
		if (buf[n - 1] == '\n') {
			eoln = TRUE;
			buf[--n] = '\0';
			if (n > 0 && buf[n - 1] == '\r')
				buf[--n] = '\0';
		}
		
		g_string_append_len (linebuf, buf, n);
	}
	
	return rv;
}

static char *
autorun_inf_get (const char *path, const char *section, const char *key)
{
	register char *inptr;
	char *value = NULL;
	GString *linebuf;
	gboolean rv;
	size_t len;
	FILE *fp;
	
	if (!(fp = fopen (path, "rt")))
		return NULL;
	
	len = strlen (key);
	linebuf = g_string_new ("");
	
	/* find the .inf section requested */
	while ((rv = readline (fp, linebuf))) {
		g_strchomp (linebuf->str);
		if (!g_ascii_strcasecmp (linebuf->str, section))
			break;
	}
	
	if (rv) {
		/* section found, read until EOF or until a new section is found */
		while ((rv = readline (fp, linebuf))) {
			if (linebuf->str[0] == '[') {
				/* beginning of a new section, key not found */
				break;
			}
			
			if (g_ascii_strncasecmp (linebuf->str, key, len) != 0) {
				/* these are not the droids you are looking for... */
				continue;
			}
			
			/* make sure the next non-lwsp char is an '=' */
			inptr = linebuf->str + len;
			while (*inptr == ' ' || *inptr == '\t')
				inptr++;
			
			if (*inptr == '\0') {
				/* correct key, but no value? */
				break;
			}
			
			if (*inptr == '=') {
				/* extract the value */
				inptr++;
				
				while (*inptr == ' ' || *inptr == '\t')
					inptr++;
				
				if (*inptr) {
					value = g_strdup (inptr);
					g_strchug (value);
				}
				
				break;
			}
		}
	}
	
	g_string_free (linebuf, TRUE);
	fclose (fp);
	
	return value;
}

static gboolean
gvm_autorun_prompt (GvmPrompt prompt, const char *udi, const char *device,
		    const char *mount_point, char *value)
{
	register char *inptr;
	char *argv[1], *path;
	GvmPromptCtx *ctx;
	GString *cmd;
	char c;
	
	if (((value[0] >= 'A' && value[0] <= 'Z')
	     || (value[0] >= 'a' && value[0] <= 'z'))
	    && value[1] == ':' && value[2] == '\\') {
		/* the autorun.inf file references the
		 * autorun program by drive (stupid,
		 * but common). */
		path = value + 3;
	} else {
		path = value;
	}
	
	/* advance to the lwsp between the path and first argument (if any) */
	inptr = path;
	while (*inptr && *inptr != ' ' && *inptr != '\t')
		inptr++;
	
	c = *inptr;
	*inptr++ = '\0';
	
	path = unix_path (mount_point, path);
	if (strncmp (path, mount_point, strlen (mount_point)) != 0) {
		/* not allowed */
		g_free (value);
		g_free (path);
		return FALSE;
	}
	
	cmd = g_string_new (path);
	
	if (c != '\0') {
		g_string_append_c (cmd, ' ');
		g_string_append (cmd, inptr);
	}
	
	g_free (value);
	
	argv[0] = path;
	ctx = gvm_prompt_ctx_new (prompt, autorun_cb, udi, device, mount_point, cmd->str);
	gvm_prompt (ctx, 1, argv);
	g_string_free (cmd, TRUE);
	g_free (path);
	
	return TRUE;
}

static gboolean
gvm_autorun_inf (const char *udi, const char *device, const char *mount_point)
{
	char *value, *path = NULL;
	struct dirent *dent;
	DIR *dir;
	
	if ((dir = opendir (mount_point))) {
		while ((dent = readdir (dir))) {
			if (!g_ascii_strcasecmp (dent->d_name, "autorun.inf")) {
				path = g_build_filename (mount_point, dent->d_name, NULL);
				break;
			}
		}
		
		closedir (dir);
	}
	
	if (!path)
		return FALSE;
	
	value = autorun_inf_get (path, "[autorun]", "open");
	g_free (path);
	
	if (!value)
		return FALSE;
	
	return gvm_autorun_prompt (GVM_PROMPT_AUTORUN, udi, device, mount_point, value);
}

static gboolean
gvm_autoopen (FILE *fp, const char *udi, const char *device, const char *mount_point)
{
	GString *str;
	char *value;
	
	str = g_string_new ("");
	if (!readline (fp, str)) {
		g_string_free (str, TRUE);
		return FALSE;
	}
	
	value = str->str;
	g_strstrip (value);
	g_string_free (str, FALSE);
	
	if (value[0] == '\0') {
		/* nothing to do */
		g_free (value);
		return FALSE;
	}
	
	return gvm_autorun_prompt (GVM_PROMPT_AUTOOPEN, udi, device, mount_point, value);
}

/*
 * gvm_autorun - automatically execute stuff
 *
 * we currently autorun: autorun files, video DVD's, and digital photos
 */
static void
gvm_autorun (const char *udi, const char *device, const char *mount_point)
{
	char **autopath, *argv[1], *path;
	gboolean handled = FALSE;
	GvmPromptCtx *ctx;
	int i;
	
	if (gvm_check_dvd (udi, device, mount_point))
		return;
	
	if (gvm_check_vcd (udi, device, mount_point))
		return;
	
	if (config.autophoto && gvm_check_photos (udi, device, mount_point)) {
		ctx = gvm_prompt_ctx_new (GVM_PROMPT_IMPORT_PHOTOS, import_photos_cb, udi, device, mount_point, NULL);
		gvm_prompt (ctx, 0, NULL);
		return;
	}
	
	if (config.autorun && config.autorun_path) {
		struct stat st;
		
		autopath = g_strsplit (config.autorun_path, ":", -1);
		
		for (i = 0; autopath[i]; i++) {
			path = g_strdup_printf ("%s/%s", mount_point, autopath[i]);
			if (access (path, F_OK | R_OK | X_OK) != 0 || stat (path, &st) == -1
			    || !S_ISREG (st.st_mode)) {
				g_free (path);
				continue;
			}
			
			argv[0] = path;
			ctx = gvm_prompt_ctx_new (GVM_PROMPT_AUTORUN, autorun_cb, udi, device, mount_point, path);
			gvm_prompt (ctx, 1, argv);
			handled = TRUE;
			g_free (path);
			break;
		}
		
		g_strfreev (autopath);
		
		if (!handled)
			handled = gvm_autorun_inf (udi, device, mount_point);
		
		if (handled)
			return;
	}
	
	if (config.autoopen && config.autoopen_path) {
		FILE *fp;
		
		autopath = g_strsplit (config.autoopen_path, ":", -1);
		
		for (i = 0; autopath[i]; i++) {
			path = g_strdup_printf ("%s/%s", mount_point, autopath[i]);
			if (!(fp = fopen (path, "rt"))) {
				g_free (path);
				continue;
			}
			
			g_free (path);
			
			handled = gvm_autoopen (fp, udi, device, mount_point);
			fclose (fp);
			break;
		}
		
		g_strfreev (autopath);
		
		if (handled)
			return;
	}
	
	if (config.autobrowse)
		gvm_run_filemanager (udi, device, mount_point);
}

static gboolean
gvm_udi_is_subfs_mount (const char *udi)
{
	int subfs = FALSE;
	char **callouts;
	int i;
	
	if ((callouts = libhal_device_get_property_strlist (hal_ctx, udi, "info.callouts.add", NULL))) {
		for (i = 0; callouts[i] != NULL; i++) {
			if (!strcmp (callouts[i], "hald-block-subfs")) {
				dbg ("subfs to handle mounting of %s; skipping\n", udi);
				subfs = TRUE;
				break;
			}
		}
		
		libhal_free_string_array (callouts);
	}
	
	return subfs;
}

static gboolean
gvm_storage_device_is_cdrom (const char *storage)
{
	gboolean is_cdrom = FALSE;
	char *media_type;
	
	if ((media_type = libhal_device_get_property_string (hal_ctx, storage, "storage.drive_type", NULL))) {
		is_cdrom = !strcmp (media_type, "cdrom");
		libhal_free_string (media_type);
	}
	
	return is_cdrom;
}

static gboolean
gvm_udi_is_cdrom (const char *udi)
{
	gboolean is_cdrom;
	char *storage;
	
	if (!(storage = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", NULL)))
		return FALSE;
	
	is_cdrom = gvm_storage_device_is_cdrom (storage);
	libhal_free_string (storage);
	
	return is_cdrom;
}

static void
import_storage_camera_cb (GvmPromptCtx *ctx, int action)
{
	switch (action) {
	case GVM_RESPONSE_IMPORT_PHOTOS:
		gvm_run_camera (ctx->udi, ctx->device, ctx->mount_point);
		break;
	case GVM_RESPONSE_BROWSE:
		gvm_run_filemanager (ctx->udi, ctx->device, ctx->mount_point);
		break;
	default:
		break;
	}
}

static void
ipod_photo_cb (GvmPromptCtx *ctx, int action)
{
	switch (action) {
	case GVM_RESPONSE_IMPORT_PHOTOS:
		gvm_run_camera (ctx->udi, ctx->device, ctx->path);
		break;
	case GVM_RESPONSE_SYNC_MUSIC:
		gvm_run_portable_media_player (ctx->udi, ctx->device, ctx->mount_point);
		break;
	default:
		break;
	}
}

/*
 * gvm_device_mounted - called once a device has been
 * mounted. Launches any user-specified applications that require the
 * device to be mounted first (Mass-Storage cameras, iPods, CDs, DVDs,
 * etc)
 *
 */
static void
gvm_device_mounted (const char *udi)
{
	char *mount_point = NULL;
	char *device = NULL;
	gboolean is_ipod = FALSE;
	GvmPromptCtx *ctx;
	DBusError error;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "block.device", &error))) {
		warn ("cannot get block.device: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		goto out;
	}
	
	if (!(mount_point = libhal_device_get_property_string (hal_ctx, udi, "volume.mount_point", &error))) {
		warn ("cannot get volume.mount_point: %s", error.message);
		if (dbus_error_is_set (&error))
			dbus_error_free (&error);
		goto out;
	}
	
	/* this is where the magic happens */
	if (config.autophoto && gvm_udi_is_storage_camera (udi)) {
		ctx = gvm_prompt_ctx_new (GVM_PROMPT_IMPORT_STORAGE_CAMERA, import_storage_camera_cb, udi, device, mount_point, NULL);
		gvm_prompt (ctx, 0, NULL);
	} else if (config.autoipod && gvm_udi_is_portable_media_player (udi, &is_ipod)) {
		char *ipod_control = NULL;
		const char *photo_dir;
		
		if (is_ipod) {
			ipod_control = g_build_filename (mount_point, "iPod_Control", NULL);
			photo_dir = ipod_control;
		} else {
			photo_dir = mount_point;
		}
		
		if (gvm_check_photos (udi, device, photo_dir)) {
			/* we have ourselves an iPod Photo - need to prompt what to do */
			ctx = gvm_prompt_ctx_new (GVM_PROMPT_IPOD_PHOTO, ipod_photo_cb, udi, device, mount_point, photo_dir);
			gvm_prompt (ctx, 0, NULL);
		} else {
			gvm_run_portable_media_player (udi, device, mount_point);
		}
		
		g_free (ipod_control);
	} else {
		gvm_autorun (udi, device, mount_point);
	}
	
 out:
	
	libhal_free_string (mount_point);
	libhal_free_string (device);
}


enum {
	MOUNT_CODEPAGE   = (1 << 0),
	MOUNT_DATA       = (1 << 1),
	MOUNT_DIRSYNC    = (1 << 2),
	MOUNT_DMASK      = (1 << 3),
	MOUNT_FMASK      = (1 << 4),
	MOUNT_FLUSH      = (1 << 5),
	MOUNT_IOCHARSET  = (1 << 6),
	MOUNT_MODE       = (1 << 7),
	MOUNT_NOATIME    = (1 << 8),
	MOUNT_NODIRATIME = (1 << 9),
	MOUNT_NOEXEC     = (1 << 10),
	MOUNT_QUIET      = (1 << 11),
	MOUNT_READ_ONLY  = (1 << 12),
	MOUNT_SHORTNAME  = (1 << 13),
	MOUNT_SYNC       = (1 << 14),
	MOUNT_UID        = (1 << 15),
	MOUNT_UMASK      = (1 << 16),
	MOUNT_UTF8       = (1 << 17),
};

static struct {
	const char *name;
	guint32 flag;
} mount_options[] = {
	{ "codepage=",  MOUNT_CODEPAGE   },  /* vfat */
	{ "data=",      MOUNT_DATA       },  /* ext3 */
	{ "dirsync",    MOUNT_DIRSYNC    },
	{ "dmask=",     MOUNT_DMASK      },  /* vfat, ntfs */
	{ "fmask=",     MOUNT_FMASK      },  /* vfat, ntfs */
	{ "flush",      MOUNT_FLUSH      },  /* vfat */
	{ "iocharset=", MOUNT_IOCHARSET  },  /* vfat, iso9660 */
	{ "mode=",      MOUNT_MODE       },  /* iso9660 */
	{ "noatime",    MOUNT_NOATIME    },
	{ "nodiratime", MOUNT_NODIRATIME },
	{ "noexec",     MOUNT_NOEXEC     },
	{ "quiet",      MOUNT_QUIET      },
	{ "ro",         MOUNT_READ_ONLY  },
	{ "shortname=", MOUNT_SHORTNAME  },  /* vfat */
	{ "sync",       MOUNT_SYNC       },
	{ "uid=",       MOUNT_UID        },  /* vfat, ntfs, udf, iso9660 */
	{ "umask=",     MOUNT_UMASK      },  /* vfat, ntfs, udf */
	{ "utf8",       MOUNT_UTF8       },  /* vfat, iso9660 */
};


static const char *
canonicalize_codeset (char *str)
{
	register char *s = str;
	register char *d = str;
	
	while (*s) {
		if (*s >= 'A' && *s <= 'Z')
			*d++ = *s++ + 0x20;
		else if (*s != '-' && *s != '_')
			*d++ = *s++;
		else
			s++;
	}
	
	*d = '\0';
	
	return str;
}

static struct {
	const char *codeset;
	const char *iocharset;
} iocharset_mapping[] = {
	{ "tis620",      "cp874"      },
	
	{ "shiftjis*",   "cp932"      },
	{ "sjis",        "cp932"      },
	
	{ "gb18030",     "cp936"      },
	{ "gbk",         "cp936"      },
	{ "gb2312",      "cp936"      },
	
	{ "euckr",       "cp949"      },
	
	{ "big5*",       "cp950"      },
	{ "euctw",       "cp950"      },
	
	/*{ "cp1251",      "cp1251"     },*/
	
	{ "eucjp",       "euc-jp"     },
	
	{ "iso88591",    "iso8859-1"  },
	{ "iso88592",    "iso8859-2"  },
	{ "iso88593",    "iso8859-3"  },
	{ "iso88594",    "iso8859-4"  },
	{ "iso88595",    "iso8859-5"  },
	{ "iso88596",    "iso8859-6"  },
	{ "iso88597",    "iso8859-7"  },
	{ "iso88599",    "iso8859-9"  },
	{ "iso885913",   "iso8859-13" },
	{ "iso885914",   "iso8859-14" },
	{ "iso885915",   "iso8859-15" },
	
	{ "koi8r",       "koi8-r"     },
	{ "koi8u",       "koi8-u"     },
	
	/*{ "utf8",       "utf8"       },*/
};

static const char *
gvm_iocharset (void)
{
	static const char *iocharset = NULL;
	char *locale, *codeset, *inptr;
	static int initialized = FALSE;
	const char *wildcard;
	size_t i, n;
	
	if (initialized)
		return iocharset;
	
	initialized = TRUE;
	
	locale = setlocale (LC_ALL, NULL);
	if (!locale || !strcmp (locale, "C") || !strcmp (locale, "POSIX")) {
		/* The locale "C"  or  "POSIX"  is  a  portable  locale;  its
		 * LC_CTYPE  part  corresponds  to  the 7-bit ASCII character
		 * set.
		 */
		
		iocharset = NULL;
	} else {
#ifdef HAVE_CODESET
		codeset = g_strdup (nl_langinfo (CODESET));
		canonicalize_codeset (codeset);
#else
		/* A locale name is typically of  the  form  language[_terri-
		 * tory][.codeset][@modifier],  where  language is an ISO 639
		 * language code, territory is an ISO 3166 country code,  and
		 * codeset  is  a  character  set or encoding identifier like
		 * ISO-8859-1 or UTF-8.
		 */
		if (!(codeset = strchr (locale, '.'))) {
			/* charset unknown */
			return NULL;
		}
		
		codeset++;
		
		/* ; is a hack for debian systems and / is a hack for Solaris systems */
		inptr = codeset;
		while (*inptr && !strchr ("@;/", *inptr))
			inptr++;
		
		codeset = g_strndup (codeset, inptr - codeset);
		canonicalize_codeset (codeset);
#endif
		
		for (i = 0; i < G_N_ELEMENTS (iocharset_mapping); i++) {
			if ((wildcard = strchr (iocharset_mapping[i].codeset, '*'))) {
				n = wildcard - iocharset_mapping[i].codeset;
				if (!strncmp (codeset, iocharset_mapping[i].codeset, n)) {
					iocharset = iocharset_mapping[i].iocharset;
					break;
				}
			} else if (!strcmp (codeset, iocharset_mapping[i].codeset)) {
				iocharset = iocharset_mapping[i].iocharset;
				break;
			}
		}
		
		if (!iocharset) {
			iocharset = codeset;
			codeset = NULL;
		}
		
		g_free (codeset);
	}
	
	return iocharset;
}


static gboolean
gvm_mount_options (GPtrArray *options, guint32 opts, const char *type, const char *where)
{
	char *option, *key, *tmp, *p;
	GSList *list, *l, *n;
	GConfClient *gconf;
	const char *dir;
	
	if (!strncmp (where, "/org/freedesktop/Hal/", 21)) {
		/* flatten the UDI */
		dir = p = tmp = g_strdup (where);
		while (*p != '\0') {
			if (*p == '/')
				*p = '_';
			p++;
		}
	} else {
		dir = where;
		tmp = NULL;
	}
	
	key = g_strdup_printf ("/system/storage/%s/%s/mount_options", type, dir);
	g_free (tmp);
	
	gconf = gconf_client_get_default ();
	list = gconf_client_get_list (gconf, key, GCONF_VALUE_STRING, NULL);
	g_object_unref (gconf);
	g_free (key);
	
	if (list == NULL) {
		fprintf (stderr, "no mount options found for %s::%s\n", type, where);
		return FALSE;
	}
	
	for (l = list; l != NULL; l = n) {
		option = l->data;
		n = l->next;
		
		g_ptr_array_add (options, option);
		
		g_slist_free_1 (l);
	}
	
	if (opts & MOUNT_UID) {
		option = g_strdup_printf ("uid=%u", getuid ());
		g_ptr_array_add (options, option);
	}
	
	return TRUE;
}


/*
 * gvm_device_mount - mount the given device.
 *
 * @return TRUE iff the mount was succesful
 */
static gboolean
gvm_device_mount (const char *udi, gboolean interactive)
{
	struct _MountPolicy *policy;
	
	dbg ("mounting %s...\n", udi);
	
	if (!gnome_mount || access (gnome_mount, F_OK | R_OK | X_OK) != 0) {
		g_free (gnome_mount);
		gnome_mount = g_find_program_in_path ("gnome-mount");
	}
	
	if (gnome_mount != NULL) {
		gboolean retval;
		char *command;
		
		policy = g_new (struct _MountPolicy, 1);
		policy->udi = g_strdup (udi);
		policy->apply = interactive;
		
		g_hash_table_insert (mount_table, policy->udi, policy);
		
		if (!interactive) {
			command = g_strdup_printf ("%s --no-ui --hal-udi=%%h", gnome_mount);
		} else {
			command = g_strdup_printf ("%s --hal-udi=%%h", gnome_mount);
		}
		
		retval = gvm_run_command (command, udi, NULL, NULL);
		g_free (command);
		
		if (!retval) {
			g_hash_table_remove (mount_table, policy->udi);
			g_free (policy->udi);
			g_free (policy);
		}
		
		return retval;
	} else {
		char *mount_point, *fstype, *drive, **moptions, fmask_opt[12], *charset_opt = NULL;
		DBusMessage *dmesg, *reply;
		gboolean freev = FALSE;
		GPtrArray *options;
		guint32 opts = 0;
		DBusError error;
		size_t i, j;
		
		if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal", udi,
							    "org.freedesktop.Hal.Device.Volume",
							    "Mount"))) {
			dbg ("mount failed for %s: could not create dbus message\n", udi);
			return FALSE;
		}
		
		if ((moptions = libhal_device_get_property_strlist (hal_ctx, udi, "volume.mount.valid_options", NULL))) {
			for (i = 0; moptions[i]; i++) {
				for (j = 0; j < G_N_ELEMENTS (mount_options); j++) {
					if (!strcmp (moptions[i], mount_options[j].name))
						opts |= mount_options[j].flag;
				}
			}
			
			libhal_free_string_array (moptions);
		}
		
		options = g_ptr_array_new ();
		
		/* check volume-specific mount options */
		if (gvm_mount_options (options, opts, "volumes", udi)) {
			freev = TRUE;
			goto mount;
		}
		
		/* check drive specific mount options */
		if ((drive = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", NULL))) {
			if (gvm_mount_options (options, opts, "drives", drive)) {
				libhal_free_string (drive);
				freev = TRUE;
				goto mount;
			}
			libhal_free_string (drive);
		}
		
		if ((fstype = libhal_device_get_property_string (hal_ctx, udi, "volume.fstype", NULL))) {
			const char *iocharset;
			char uid[32];
			mode_t mask;
			
			/* fall back to using fstype-specific mount options */
			if (gvm_mount_options (options, opts, "default_options", fstype)) {
				libhal_free_string (fstype);
				freev = TRUE;
				goto mount;
			} else if (gvm_mount_options (options, opts, "default_options", "*")) {
				libhal_free_string (fstype);
				freev = TRUE;
				goto mount;
			}
			
			/* take our best guess at what the user would want */
			if (!strcmp (fstype, "vfat")) {
				if (opts & MOUNT_NOEXEC)
					g_ptr_array_add (options, "noexec");
				
				/* preferably mount flush (opportunistic syncing) if available, else
				 * mount sync for "small" volumes (that are likely to be thumb drives)
				 */
				if (opts & MOUNT_FLUSH) {
					g_ptr_array_add (options, "flush");
				} else if (opts & MOUNT_SYNC) {
					dbus_uint64_t size;
					
					size = libhal_device_get_property_uint64 (hal_ctx, udi, "volume.size", NULL);
					if (size <= (512 * 1024 * 1024))
						g_ptr_array_add (options, "sync");
				}
				
				if (opts & MOUNT_FMASK) {
					mask = umask (0);
					snprintf (fmask_opt, sizeof (fmask_opt), "fmask=%#o", mask | 0111);
					g_ptr_array_add (options, fmask_opt);
					umask (mask);
				}
				
				if (opts & MOUNT_SHORTNAME)
					g_ptr_array_add (options, "shortname=lower");
			} else if (!strcmp (fstype, "iso9660")) {
				/* only care about uid= and iocharset= */
			} else if (!strcmp (fstype, "udf")) {
				/* also care about uid= and iocharset= */
				if (opts & MOUNT_NOATIME)
					g_ptr_array_add (options, "noatime");
			}
			
			if (opts & (MOUNT_IOCHARSET|MOUNT_UTF8)) {
				if ((iocharset = gvm_iocharset ())) {
					if ((opts & MOUNT_UTF8) && !strcmp (iocharset, "utf8")) {
						g_ptr_array_add (options, "utf8");
					} else if (opts & MOUNT_IOCHARSET) {
						charset_opt = g_strdup_printf ("iocharset=%s", iocharset);
						g_ptr_array_add (options, charset_opt);
					}
				}
			}
			
			if (opts & MOUNT_UID) {
				snprintf (uid, sizeof (uid) - 1, "uid=%u", getuid ());
				g_ptr_array_add (options, uid);
			}
			
			libhal_free_string (fstype);
		}
		
	mount:
		
		mount_point = "";
		fstype = "";
		
		if (!dbus_message_append_args (dmesg, DBUS_TYPE_STRING, &mount_point, DBUS_TYPE_STRING, &fstype,
					       DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options->pdata, options->len,
					       DBUS_TYPE_INVALID)) {
			dbg ("mount failed for %s: could not append args to dbus message\n", udi);
			dbus_message_unref (dmesg);
			return FALSE;
		}
		
		if (freev) {
			for (i = 0; i < options->len; i++)
				g_free (options->pdata[i]);
		}
		
		g_ptr_array_free (options, TRUE);
		g_free (charset_opt);
		
		policy = g_new (struct _MountPolicy, 1);
		policy->udi = g_strdup (udi);
		policy->apply = interactive;
		
		g_hash_table_insert (mount_table, policy->udi, policy);
		
		dbus_error_init (&error);
		if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
			dbg ("mount failed for %s: %s\n", udi, error.message);
			g_hash_table_remove (mount_table, policy->udi);
			dbus_message_unref (dmesg);
			dbus_error_free (&error);
			g_free (policy->udi);
			g_free (policy);
			return FALSE;
		}
		
		dbg ("mount queued for %s\n", udi);
		
		dbus_message_unref (dmesg);
		dbus_message_unref (reply);
		
		return TRUE;
	}
}


/*
 * gvm_device_unmount - unmount the given device.
 *
 * @return TRUE iff the unmount was succesful
 */
static gboolean
gvm_device_unmount (const char *udi)
{
	DBusMessage *dmesg, *reply;
	char **options = NULL;
	DBusError error;
	gboolean retval;
	char *command;
	
	dbg ("unmounting %s...\n", udi);
	
	if (!gnome_mount || access (gnome_mount, F_OK | R_OK | X_OK) != 0) {
		g_free (gnome_mount);
		gnome_mount = g_find_program_in_path ("gnome-mount");
	}
	
	if (gnome_mount != NULL) {
		command = g_strdup_printf ("%s --unmount --hal-udi=%%h", gnome_mount);
		retval = gvm_run_command (command, udi, NULL, NULL);
		g_free (command);
		
		return retval;
	} else {
		if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal", udi,
							    "org.freedesktop.Hal.Device.Volume",
							    "Unmount"))) {
			dbg ("unmount failed for %s: could not create dbus message\n", udi);
			return FALSE;
		}
		
		if (!dbus_message_append_args (dmesg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options, 0,
					       DBUS_TYPE_INVALID)) {
			dbg ("unmount failed for %s: could not append args to dbus message\n", udi);
			dbus_message_unref (dmesg);
			return FALSE;
		}
		
		dbus_error_init (&error);
		if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
			dbg ("unmount failed for %s: %s\n", udi, error.message);
			dbus_message_unref (dmesg);
			dbus_error_free (&error);
			return FALSE;
		}
		
		dbg ("unmount queued for %s\n", udi);
		
		dbus_message_unref (dmesg);
		dbus_message_unref (reply);
		
		return TRUE;
	}
}

/*
 * gvm_run_cdplay - if so configured, execute the user-specified CD player on
 * the given device node
 */
static void
gvm_run_cdplayer (const char *udi, const char *device, const char *mount_point)
{
	if (config.autoplay_cda_command != NULL)
		gvm_run_command (config.autoplay_cda_command, udi, device, mount_point);
}

static void
cda_extra_cb (GvmPromptCtx *ctx, int action)
{
	switch (action) {
	case GVM_RESPONSE_BROWSE:
		if (!gvm_udi_is_subfs_mount (ctx->udi))
			gvm_device_mount (ctx->udi, TRUE);
		break;
	case GVM_RESPONSE_PLAY:
		gvm_run_cdplayer (ctx->udi, ctx->device, NULL);
		break;
	default:
		break;
	}
}

/*
 * gvm_ask_mixed - if a mixed mode CD (CD Plus) is inserted, we can either
 * mount the data tracks or play the audio tracks.  How we handle that depends
 * on the user's configuration.  If the configuration allows either option,
 * we ask.
 */
static void
gvm_ask_mixed (const char *udi)
{
	char *device = NULL;
	GvmPromptCtx *ctx;
	DBusError error;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "block.device", &error))) {
		warn ("cannot get block.device: %s", error.message);
		dbus_error_free (&error);
		return;
	}
	
	if (config.automount_media && config.autoplay_cda) {
		ctx = gvm_prompt_ctx_new (GVM_PROMPT_CDA_EXTRA, cda_extra_cb, udi, device, NULL, NULL);
		gvm_prompt (ctx, 0, NULL);
	} else if (config.automount_media) {
		if (!gvm_udi_is_subfs_mount (udi))
			gvm_device_mount (udi, TRUE);
	} else if (config.autoplay_cda) {
		gvm_run_cdplayer (udi, device, NULL);
	}
	
	libhal_free_string (device);
}


typedef enum {
	WRITER_TYPE_NONE,
	WRITER_TYPE_CDR,
	WRITER_TYPE_DVD
} writer_t;

static struct {
	const char *disc;
	const char *drive;
	writer_t type;
} burners[] = {
	{ "cd_r",        "storage.cdrom.cdr",       WRITER_TYPE_CDR },
	{ "cd_rw",       "storage.cdrom.cdrw",      WRITER_TYPE_CDR },
	{ "dvd_r",       "storage.cdrom.dvdr",      WRITER_TYPE_DVD },
	{ "dvd_rw",      "storage.cdrom.dvdrw",     WRITER_TYPE_DVD },
	{ "dvd_ram",     "storage.cdrom.dvdram",    WRITER_TYPE_DVD },
	{ "dvd_plus_r",  "storage.cdrom.dvdplusr",  WRITER_TYPE_DVD },
	{ "dvd_plus_rw", "storage.cdrom.dvdplusrw", WRITER_TYPE_DVD },
};

/*
 * gvm_cdrom_media_is_writable - returns the type of media that can be written
 */
static writer_t
gvm_cdrom_media_is_writable (const char *udi)
{
	writer_t retval = WRITER_TYPE_NONE;
	char *drive = NULL;
	char *disc = NULL;
	size_t i;
	
	if (!(disc = libhal_device_get_property_string (hal_ctx, udi, "volume.disc.type", NULL)))
		return FALSE;
	
	for (i = 0; i < G_N_ELEMENTS (burners); i++) {
		if (!strcmp (burners[i].disc, disc)) {
			if (!(drive = libhal_device_get_property_string (hal_ctx, udi, "info.parent", NULL)))
				break;
			
			if (libhal_device_get_property_bool (hal_ctx, drive, burners[i].drive, NULL))
				retval = burners[i].type;
			
			break;
		}
	}
	
	libhal_free_string (drive);
	libhal_free_string (disc);
	
	return retval;
}

static void
burn_cdr_cb (GvmPromptCtx *ctx, int action)
{
	const char *command;
	
	switch (action) {
	case GVM_RESPONSE_WRITE_AUDIO_CD:
		command = config.autoburn_audio_cd_command;
		break;
	case GVM_RESPONSE_WRITE_DATA_CD:
		command = config.autoburn_data_cd_command;
		break;
	default:
		return;
	}
	
	gvm_run_command (command, ctx->udi, ctx->device, ctx->mount_point);
}

/*
 * gvm_run_cdburner - execute the user-specified CD burner command on the
 * given device node, if so configured
 */
static void
gvm_run_cdburner (const char *udi, int type, const char *device, const char *mount_point)
{
	GvmPromptCtx *ctx;
	GvmPrompt prompt;
	
	if (!config.autoburn)
		return;
	
	if (type == WRITER_TYPE_DVD)
		prompt = GVM_PROMPT_WRITE_DVD;
	else
		prompt = GVM_PROMPT_WRITE_CDR;
	
	ctx = gvm_prompt_ctx_new (prompt, burn_cdr_cb, udi, device, mount_point, NULL);
	gvm_prompt (ctx, 0, NULL);
}


/*
 * gvm_cdrom_policy - There has been a media change event on the CD-ROM
 * associated with the given UDI.  Enforce policy.
 */
static void
gvm_cdrom_policy (const char *udi)
{
	dbus_bool_t has_audio;
	dbus_bool_t has_data;
	char *device = NULL;
	DBusError error;
	writer_t type;
	
	dbus_error_init (&error);
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "block.device", &error))) {
		warn ("cannot get block.device: %s", error.message);
		dbus_error_free (&error);
		return;
	}
	
	if (libhal_device_get_property_bool (hal_ctx, udi, "volume.disc.is_blank", NULL)) {
		if ((type = gvm_cdrom_media_is_writable (udi)))
			gvm_run_cdburner (udi, type, device, NULL);
	} else {
		has_audio = libhal_device_get_property_bool (hal_ctx, udi, "volume.disc.has_audio", NULL);
		has_data = libhal_device_get_property_bool (hal_ctx, udi, "volume.disc.has_data", NULL);
		
		if (has_audio && has_data) {
			gvm_ask_mixed (udi);
		} else if (has_audio) {
			if (config.autoplay_cda)
				gvm_run_cdplayer (udi, device, NULL);
		} else if (has_data) {
			if (config.automount_media && !gvm_udi_is_subfs_mount (udi))
				gvm_device_mount (udi, TRUE);
		}
	}
	
	/** @todo enforce policy for all the new disc types now supported */
	
	libhal_free_string (device);
}

/*
 * gvm_media_changed - generic media change handler.
 *
 * This is called on a UDI and the media's parent device in response to a media
 * change event.  We have to decipher the storage media type to run the
 * appropriate media-present check.  Then, if there is indeed media in the
 * drive, we enforce the appropriate policy.
 *
 * At the moment, we only handle CD-ROM and DVD drives.
 *
 * Returns TRUE if the device was handled or FALSE otherwise
 */
static gboolean
gvm_media_changed (const char *udi, const char *storage_device)
{
	gboolean handled = FALSE;
	DBusError error;
	
	/* Refuse to enforce policy on removable media if drive is locked */
	dbus_error_init (&error);
	if (libhal_device_property_exists (hal_ctx, storage_device, "info.locked", NULL)
	    && libhal_device_get_property_bool (hal_ctx, storage_device, "info.locked", NULL)) {
		dbg ("Drive with udi %s is locked through hal; skipping policy\n", storage_device);
		/* we return TRUE here because the device is locked - we can pretend we handled it */
		return TRUE;
	}
	
	if (gvm_storage_device_is_cdrom (storage_device)) {
		gvm_cdrom_policy (udi);
		handled = TRUE;
	}
	
	return handled;
}


static void
import_camera_cb (GvmPromptCtx *ctx, int action)
{
	if (action == GVM_RESPONSE_IMPORT_PHOTOS)
		gvm_run_camera (ctx->udi, NULL, NULL);
}


static int
strptrcmp (const void *strptr0, const void *strptr1)
{
	return strcmp (*((const char **) strptr0), *((const char **) strptr1));
}


typedef gboolean (* DeviceAddedHandler) (const char *udi, const char *capability);

static gboolean
block_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	char *fsusage = NULL, *device = NULL, *storage_device = NULL;
	DBusError error;
	int mountable;
	int crypto;
	
	dbus_error_init (&error);
	
	crypto = FALSE;
	
	/* is this a mountable volume? */
	if (!(mountable = libhal_device_get_property_bool (hal_ctx, udi, "block.is_volume", NULL))) {
		dbg ("not a mountable volume: %s\n", udi);
		goto out;
	}
	
	/* if it is a volume, it must have a device node */
	if (!(device = libhal_device_get_property_string (hal_ctx, udi, "block.device", &error))) {
		dbg ("cannot get block.device: %s\n", error.message);
		goto out;
	}
	
	if (mountable) {
		/* only mount if the block device has a sensible filesystem */
		if (!(fsusage = libhal_device_get_property_string (hal_ctx, udi, "volume.fsusage", &error))) {
			dbg ("unable to get fsusage for %s: %s\n", udi, error.message);
			mountable = FALSE;
		} else if (!strcmp (fsusage, "crypto")) {
			dbg ("encrypted volume found: %s\n", udi);
			/* TODO: handle encrypted volumes */
			mountable = FALSE;
			crypto = TRUE;
		} else if (strcmp (fsusage, "filesystem") != 0) {
			dbg ("no sensible filesystem for %s\n", udi);
			mountable = FALSE;
		}
	}
	
	/* get the backing storage device */
	if (!(storage_device = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", &error))) {
		dbg ("cannot get block.storage_device: %s\n", error.message);
		goto out;
	}
	
	/* if the partition_table_changed flag is set, we don't want
	 * to mount as a partitioning tool might be modifying this
	 * device */
	if (libhal_device_get_property_bool (hal_ctx, storage_device, "storage.partition_table_changed", NULL)) {
		dbg ("partition table changed for %s\n", storage_device);
		goto out;
	}
	
	/*
	 * Does this device support removable media?  Note that we
	 * check storage_device and not our own UDI
	 */
	if (libhal_device_get_property_bool (hal_ctx, storage_device, "storage.removable", NULL)) {
		/* we handle media change events separately */
		dbg ("Changed: %s\n", device);
		if (gvm_media_changed (udi, storage_device))
			goto out;
	}
	
	if (config.automount_drives && (mountable || crypto)) {
		if (!gvm_udi_is_subfs_mount (udi)) {
			if (libhal_device_get_property_bool (hal_ctx, udi, "volume.ignore", NULL)) {
				dbg ("volume.ignore set to true on %s, not mounting\n", udi);
                        } else if (!libhal_device_get_property_bool (hal_ctx, storage_device, "storage.automount_enabled_hint", NULL)) {
				dbg ("automounting disabled for %s, not mounting\n", storage_device);
			} else {
				gvm_device_mount (udi, TRUE);
			}
		}
	}
	
 out:
	
	if (dbus_error_is_set (&error))
		dbus_error_free (&error);
	
	libhal_free_string (device);
	libhal_free_string (fsusage);
	libhal_free_string (storage_device);
	
	return TRUE;
}

static gboolean
camera_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	GvmPromptCtx *ctx;
	
	/* check that the camera is a non-storage camera */
	if (!gvm_udi_is_camera (udi))
		return TRUE;
	
	if (!(config.autophoto && config.autophoto_command))
		return FALSE;
	
	ctx = gvm_prompt_ctx_new (GVM_PROMPT_IMPORT_CAMERA, import_camera_cb, udi, NULL, NULL, NULL);
	gvm_prompt (ctx, 0, NULL);
	
	return TRUE;
}

static struct {
	const char *capability;
	gboolean *autoexec;
	char **command;
} inputs[] = {
	{ "input.keyboard", &config.autokeyboard, &config.autokeyboard_command },
	{ "input.mouse",    &config.automouse,    &config.automouse_command    },
	{ "input.tablet",   &config.autotablet,   &config.autotablet_command   },
};

static gboolean
input_device_added (const char *udi, const char *capability)
{
	/* input device (keyboard, mouse, wacom tablet, etc...) */
	const char *command = NULL;
	int autoexec = FALSE;
	DBusError error;
	char *device;
	size_t i;
	
	for (i = 0; i < G_N_ELEMENTS (inputs); i++) {
		if (!strcmp (inputs[i].capability, capability)) {
			autoexec = *inputs[i].autoexec;
			command = *inputs[i].command;
			break;
		}
	}
	
	if (i == G_N_ELEMENTS (inputs)) {
		/* we don't handle this type of device */
		return FALSE;
	}
	
	if (autoexec && command) {
		dbus_error_init (&error);
		if ((device = libhal_device_get_property_string (hal_ctx, udi, "input.device", &error))) {
			gvm_run_command (command, udi, device, NULL);
			libhal_free_string (device);
		} else {
			warn ("cannot get input.device property: %s", error.message);
			dbus_error_free (&error);
		}
		
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
pda_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	DBusError error;
	char *platform;
	
	dbus_error_init (&error);
	
	if (!(platform = libhal_device_get_property_string (hal_ctx, udi, "pda.platform", &error))) {
		warn ("cannot get pda.platform property: %s", error.message);
		dbus_error_free (&error);
		return TRUE;
	}
	
	if (!strcmp (platform, "palm")) {
		if (config.autopilot)
			gvm_run_pilot (udi);
	} else if (!strcmp (platform, "pocketpc")) {
		if (config.autopocketpc)
			gvm_run_pocketpc (udi);
	}
	
	libhal_free_string (platform);
	
	return TRUE;
}

static gboolean
media_player_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	char *access_method;
	DBusError error;
	
	dbus_error_init (&error);
	
	if (!(access_method = libhal_device_get_property_string (hal_ctx, udi, "portable_audio_player.access_method", &error))) {
		warn ("cannot get portable_audio_player.access_method property: %s", error.message);
		dbus_error_free (&error);
		return TRUE;
	}
	
	if (!strcmp (access_method, "storage")) {
		/* these get handled after being mounted */
		g_free (access_method);
		return TRUE;
	}
	
	g_free (access_method);
	
	if (config.autoipod)
		gvm_run_portable_media_player (udi, NULL, NULL);
	
	return TRUE;
}

static gboolean
printer_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	if (config.autoprinter) {
		gvm_run_printer (udi);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
scanner_device_added (const char *udi, const char *capability GNUC_UNUSED)
{
	if (config.autoscanner) {
		gvm_run_scanner (udi);
		return TRUE;
	}
	
	return FALSE;
}

static gboolean
webcam_device_added (const char *udi, const char *capability __attribute__((__unused__)))
{
	if (config.autowebcam) {
		gvm_run_webcam (udi);
		return TRUE;
	}
	
	return FALSE;
}

/* Note: this list must be sorted alphabetically for the algorithm in hal_device_added to work */
static struct {
	const char *capability;
	DeviceAddedHandler handler;
} devices[] = {
	{ "block",                 block_device_added        },
	{ "camera",                camera_device_added       },
	/*{ "input",                 input_device_added        },*/ /* we don't handle generic input devices (yet?) */
	{ "input.keyboard",        input_device_added        },
	{ "input.mouse",           input_device_added        },
	{ "input.tablet",          input_device_added        },
	{ "pda",                   pda_device_added          },
	{ "portable_audio_player", media_player_device_added },
	{ "printer",               printer_device_added      },
	{ "scanner",               scanner_device_added      },
	{ "video4linux",           webcam_device_added       },
};

/** Invoked when a device is added to the Global Device List. 
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Universal Device Id
 */
static void
hal_device_added (LibHalContext *ctx GNUC_UNUSED,
		  const char *udi)
{
	char **capabilities;
	size_t i, j, n;
	
	if (!gvm_user_is_active ())
		return;
	
	dbg ("Device added: %s\n", udi);
	
	if (!(capabilities = libhal_device_get_property_strlist (hal_ctx, udi, "info.capabilities", NULL)))
		return;
	
	for (n = 0; capabilities[n]; n++)
		;
	
	qsort (capabilities, n, sizeof (char *), strptrcmp);
	
	for (i = 0, j = 0; i < G_N_ELEMENTS (devices) && j < n; i++) {
		int cmp = -1;
		
		while (j < n && (cmp = strcmp (capabilities[j], devices[i].capability)) < 0)
			j++;
		
		if (cmp == 0) {
			if (devices[i].handler (udi, capabilities[j]))
				break;
			j++;
		}
	}
	
	libhal_free_string_array (capabilities);
}

/** Invoked when a device is removed from the Global Device List. 
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Universal Device Id
 */
static void
hal_device_removed (LibHalContext *ctx GNUC_UNUSED, 
		    const char *udi)
{
	GtkDialog *dialog;
	char *device_udi;
	
	dbg ("Device removed: %s\n", udi);
	
	/* unmounted leftover mount point */
	if ((device_udi = g_hash_table_lookup (device_table, udi))) {
		if (gvm_device_unmount (device_udi)) {
			GSList *l, *n;
					
			for (l = mounted_volumes; l != NULL; l = n) {
				n = l->next;
				if (strcmp (udi, (const char *) l->data) == 0) {
					g_free (l->data);
					mounted_volumes = g_slist_delete_link (mounted_volumes, l);
					break;
				}
			}
		}
	}
	
	if ((dialog = g_hash_table_lookup (dialogs, udi)))
		gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

/** Invoked when device in the Global Device List acquires a new capability.
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Universal Device Id
 *  @param  capability          Name of capability
 */
static void
hal_device_new_capability (LibHalContext *ctx GNUC_UNUSED,
			   const char *udi GNUC_UNUSED, 
			   const char *capability GNUC_UNUSED)
{
}

/** Invoked when device in the Global Device List loses a capability.
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Universal Device Id
 *  @param  capability          Name of capability
 */
static void
hal_device_lost_capability (LibHalContext *ctx GNUC_UNUSED,
			    const char *udi GNUC_UNUSED, 
			    const char *capability GNUC_UNUSED)
{
}

/** Invoked when a property of a device in the Global Device List is
 *  changed, and we have subscribed to changes for that device.
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Univerisal Device Id
 *  @param  key                 Key of property
 */
static void
hal_property_modified (LibHalContext *ctx GNUC_UNUSED,
		       const char *udi, 
		       const char *key,
		       dbus_bool_t is_removed GNUC_UNUSED, 
		       dbus_bool_t is_added GNUC_UNUSED)
{
	struct _MountPolicy *policy;
	gboolean mounted;
	GSList *l, *n;
	
	if (strcmp (key, "volume.is_mounted") != 0)
		return;
	
	mounted = libhal_device_get_property_bool (hal_ctx, udi, key, NULL);
	
	if (mounted) {
		dbg ("Mounted: %s\n", udi);
		
		if ((policy = g_hash_table_lookup (mount_table, udi))) {
			char *device;
			
			g_hash_table_remove (mount_table, udi);
			
			/* add to list of all volumes mounted during lifetime */
			mounted_volumes = g_slist_append (mounted_volumes, g_strdup (udi));
			
			if ((device = libhal_device_get_property_string (hal_ctx, udi, "block.storage_device", NULL)))
				g_hash_table_insert (device_table, g_strdup (udi), device);
			
			if (policy->apply)
				gvm_device_mounted (udi);
			
			g_free (policy->udi);
			g_free (policy);
		} else if (gvm_user_is_active ()) {
			dbg ("not in mount queue: %s\n", udi);
			/*gvm_device_mounted (udi);*/
		}
		
#ifdef ENABLE_NOTIFY
		statfs_mount_info_add (udi);
#endif
	} else {
		dbg ("Unmounted: %s\n", udi);
		
#ifdef ENABLE_NOTIFY
		/* remove the udi from the statfs_mounts list */
		statfs_mount_info_remove (udi);
#endif
		
		g_hash_table_remove (device_table, udi);
		
		/* unmount all volumes mounted during lifetime */
		
		for (l = mounted_volumes; l != NULL; l = n) {
			n = l->next;
			if (strcmp (udi, (const char *) l->data) == 0) {
				g_free (l->data);
				mounted_volumes = g_slist_delete_link (mounted_volumes, l);
				break;
			}
		}
	}
}

static void
gvm_device_eject (const char *udi)
{
	char *storage, **volumes = NULL;
	DBusMessage *dmesg, *reply;
	const char *volume = udi;
	char **options = NULL;
	DBusError error;
	char *command;
	int i, n;
	
	if (!libhal_device_get_property_bool (hal_ctx, udi, "block.is_volume", NULL)) {
		dbus_error_init (&error);
		volumes = libhal_find_device_by_capability (hal_ctx, "volume", &n, &error);
		if (dbus_error_is_set (&error)) {
			dbus_error_free (&error);
			return;
		}
		
		volume = NULL;
		for (i = 0; i < n; i++) {
			volume = volumes[i];
			
			if (!(storage = libhal_device_get_property_string (hal_ctx, volume, "info.parent", NULL)))
				continue;
			
			if (!strcmp (udi, storage)) {
				libhal_free_string (storage);
				break;
			}
			
			libhal_free_string (storage);
		}
	}
	
	dbg ("ejecting %s...\n", volume);
	
	if (!gnome_mount || access (gnome_mount, F_OK | R_OK | X_OK) != 0) {
		g_free (gnome_mount);
		gnome_mount = g_find_program_in_path ("gnome-mount");
	}
	
	if (gnome_mount != NULL) {
		command = g_strdup_printf ("%s --eject --hal-udi=%%h", gnome_mount);
		gvm_run_command (command, volume, NULL, NULL);
		g_free (command);
	} else {
		if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal", volume,
							    "org.freedesktop.Hal.Device.Volume",
							    "Eject"))) {
			goto done;
		}
		
		if (!dbus_message_append_args (dmesg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &options, 0,
					       DBUS_TYPE_INVALID)) {
			dbg ("eject failed for %s: could not append args to dbus message\n", udi);
			dbus_message_unref (dmesg);
			goto done;
		}
		
		dbus_error_init (&error);
		if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
			dbg ("eject failed for %s: %s\n", volume, error.message);
			dbus_message_unref (dmesg);
			dbus_error_free (&error);
			goto done;
		}
		
		dbus_message_unref (dmesg);
		dbus_message_unref (reply);
	}
	
 done:
	
	if (volumes)
		libhal_free_string_array (volumes);
}

/** Invoked when a device in the GDL emits a condition that cannot be
 *  expressed in a property (like when the processor is overheating)
 *
 *  @param  ctx                 LibHal context
 *  @param  udi                 Univerisal Device Id
 *  @param  condition_name      Name of condition
 *  @param  message             D-BUS message with parameters
 */
static void
hal_device_condition (LibHalContext *ctx GNUC_UNUSED,
		      const char *udi, const char *condition_name,
		      const char *condition_details GNUC_UNUSED)
{
	if (!gvm_user_is_active ())
		return;
	
	if (!strcmp (condition_name, "EjectPressed"))
		gvm_device_eject (udi);
}

static gboolean
reinit_dbus (gpointer user_data GNUC_UNUSED)
{
	if (gvm_dbus_init ()) {
		/* dbus daemon is up, get a hal context */
		if ((hal_ctx = gvm_hal_init ()))
			libhal_ctx_set_dbus_connection (hal_ctx, dbus_connection);
		else
			exit (1);
		
		return FALSE;
	}
	
	/* dbus deamon not back up yet */
	
	return TRUE;
}

static DBusHandlerResult
gvm_dbus_filter_function (DBusConnection *connection GNUC_UNUSED, DBusMessage *message, void *user_data GNUC_UNUSED)
{
	if (dbus_message_is_signal (message, DBUS_INTERFACE_LOCAL, "Disconnected") &&
	    strcmp (dbus_message_get_path (message), DBUS_PATH_LOCAL) == 0) {
		libhal_ctx_free (hal_ctx);
		hal_ctx = NULL;
		
		dbus_connection_unref (dbus_connection);
		dbus_connection = NULL;
		
		g_timeout_add (3000, reinit_dbus, NULL);
		
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
gvm_dbus_init (void)
{
	DBusError error;
	
	if (dbus_connection != NULL)
		return TRUE;
	
	dbus_error_init (&error);
	if (!(dbus_connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error))) {
		dbg ("could not get system bus: %s\n", error.message);
		dbus_error_free (&error);
		return FALSE;
	}
	
	dbus_connection_setup_with_g_main (dbus_connection, NULL);
	dbus_connection_set_exit_on_disconnect (dbus_connection, FALSE);
	
	dbus_connection_add_filter (dbus_connection, gvm_dbus_filter_function, NULL, NULL);
	
	return TRUE;
}


static void
gvm_hal_claim_branch (const char *udi)
{
	const char *claimed_by = "gnome-volume-manager";
	DBusMessage *dmesg, *reply;
	DBusError error;
	
	if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.Hal",
						    "/org/freedesktop/Hal/Manager",
						    "org.freedesktop.Hal.Manager",
						    "ClaimBranch"))) {
		return;
	}
	
	if (!dbus_message_append_args (dmesg, DBUS_TYPE_STRING, &udi, DBUS_TYPE_STRING, &claimed_by,
				       DBUS_TYPE_INVALID)) {
		dbus_message_unref (dmesg);
		return;
	}
	
	dbus_error_init (&error);
	if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
		dbus_message_unref (dmesg);
		dbus_error_free (&error);
		return;
	}
	
	dbus_message_unref (dmesg);
	dbus_message_unref (reply);
}


/** Internal HAL initialization function
 *
 * @return			The LibHalContext of the HAL connection or
 *				NULL on error.
 */
static LibHalContext *
gvm_hal_init (void)
{
	LibHalContext *ctx;
	DBusError error;
	char **devices;
	int nr;
	
	if (!gvm_dbus_init ())
		return NULL;
	
	if (!(ctx = libhal_ctx_new ())) {
		warn ("failed to create a HAL context!");
		return NULL;
	}
	
	libhal_ctx_set_dbus_connection (ctx, dbus_connection);
	
	libhal_ctx_set_device_added (ctx, hal_device_added);
	libhal_ctx_set_device_removed (ctx, hal_device_removed);
	libhal_ctx_set_device_new_capability (ctx, hal_device_new_capability);
	libhal_ctx_set_device_lost_capability (ctx, hal_device_lost_capability);
	libhal_ctx_set_device_property_modified (ctx, hal_property_modified);
        /* only needed for eject ATM, which is done by nautilus; disable
	libhal_ctx_set_device_condition (ctx, hal_device_condition);
        */
	
	dbus_error_init (&error);
	if (!libhal_device_property_watch_all (ctx, &error)) {
		warn ("failed to watch all HAL properties: %s", error.message ? error.message : "unknown");
		dbus_error_free (&error);
		libhal_ctx_free (ctx);
		return NULL;
	}
	
	if (!libhal_ctx_init (ctx, &error)) {
		warn ("libhal_ctx_init failed: %s", error.message ? error.message : "unknown");
		dbus_error_free (&error);
		libhal_ctx_free (ctx);
		return NULL;
	}
	
	/*
	 * Do something to ping the HAL daemon - the above functions will
	 * succeed even if hald is not running, so long as DBUS is.  But we
	 * want to exit silently if hald is not running, to behave on
	 * pre-2.6 systems.
	 */
	if (!(devices = libhal_get_all_devices (ctx, &nr, &error))) {
		warn ("seems that HAL is not running: %s", error.message ? error.message : "unknown");
		dbus_error_free (&error);
		
		libhal_ctx_shutdown (ctx, NULL);
		libhal_ctx_free (ctx);
		return NULL;
	}
	
	libhal_free_string_array (devices);
	
	gvm_hal_claim_branch ("/org/freedesktop/Hal/devices/local");

	return ctx;
}


/** Attempt to mount all volumes; should be called on startup.
 *
 *  @param  ctx                 LibHal context
 */
static void
mount_all (LibHalContext *ctx)
{
	char *prop, *dev, *udi, *drive;
	int num_volumes, mount;
	char **volumes;
	DBusError error;
	int i;
	
	if (!config.automount_media)
		return;
	
	dbus_error_init (&error);
	volumes = libhal_find_device_by_capability (ctx, "volume", &num_volumes, &error);
	if (dbus_error_is_set (&error)) {
		warn ("mount_all: could not find volume devices: %s", error.message);
		dbus_error_free (&error);
		return;
	}
	
	for (i = 0; i < num_volumes; i++) {
		udi = volumes[i];

		if (gvm_udi_is_subfs_mount (udi)) {
#ifdef ENABLE_NOTIFY
			statfs_mount_info_add (udi);
#endif
			
			/* monitor them for unmounting, just in case */
			if ((dev = libhal_device_get_property_string (ctx, udi, "block.storage_device", NULL)))
				g_hash_table_insert (device_table, g_strdup (udi), dev);
			
			continue;
		}
		
		/* don't attempt to mount already mounted volumes */
		if (!libhal_device_property_exists (ctx, udi, "volume.is_mounted", NULL)
		    || libhal_device_get_property_bool (ctx, udi, "volume.is_mounted", NULL)) {
#ifdef ENABLE_NOTIFY
			statfs_mount_info_add (udi);
#endif
			continue;
		}
		
		/* only mount if the block device has a sensible filesystem */
		if (!libhal_device_property_exists (ctx, udi, "volume.fsusage", NULL))
			continue;
		prop = libhal_device_get_property_string (ctx, udi, "volume.fsusage", NULL);
		if (!prop || ((strcmp (prop, "filesystem") != 0) && (strcmp (prop, "crypto") != 0))) {
			libhal_free_string (prop);
			continue;
		}
		libhal_free_string (prop);
		
		/* check our mounting policy */
		if (!(drive = libhal_device_get_property_string (ctx, udi, "info.parent", NULL)))
			continue;
		
		if (libhal_device_property_exists (ctx, drive, "storage.hotpluggable", NULL)
		    && libhal_device_get_property_bool (ctx, drive, "storage.hotpluggable", NULL))
			mount = config.automount_drives;
		else if (libhal_device_property_exists (ctx, drive, "storage.removable", NULL)
			 && libhal_device_get_property_bool (ctx, drive, "storage.removable", NULL))
			mount = config.automount_media;
		else
			mount = !libhal_device_get_property_bool (ctx, udi, "volume.ignore", NULL) &&
                            libhal_device_get_property_bool (ctx, drive, "storage.automount_enabled_hint", NULL);

		libhal_free_string (drive);
		
		if (!mount)
			continue;
		
		/* mount the device */
		if ((dev = libhal_device_get_property_string (ctx, udi, "block.device", &error))) {
			dbg ("mount_all: mounting %s\n", dev);
			/* don't make the mount program put up error dialogs */
			gvm_device_mount (udi, FALSE);
			libhal_free_string (dev);
		} else {
			warn ("mount_all: no device for udi=%s: %s", udi, error.message);
			if (dbus_error_is_set (&error))
				dbus_error_free (&error);
		}
	}
	
	libhal_free_string_array (volumes);
}

/** Unmount all volumes that were mounted during the lifetime of this
 *  g-v-m instance
 *
 *  @param  ctx                 LibHal context
 */
static void
unmount_all (void)
{
	GSList *l;
	
	dbg ("unmounting all volumes that we mounted in our lifetime\n");
	
	for (l = mounted_volumes; l != NULL; l = l->next) {
		const char *udi = l->data;
		
		dbg ("unmount_all: unmounting %s...\n", udi);
		gvm_device_unmount (udi);
	}
}


static int sigterm_unix_signal_pipe_fds[2];
static GIOChannel *sigterm_iochn;

static void 
handle_sigterm (int value GNUC_UNUSED)
{
	static char marker[1] = {'S'};

	/* write a 'S' character to the other end to tell about
	 * the signal. Note that 'the other end' is a GIOChannel thingy
	 * that is only called from the mainloop - thus this is how we
	 * defer this since UNIX signal handlers are evil
	 *
	 * Oh, and write(2) is indeed reentrant */
	write (sigterm_unix_signal_pipe_fds[1], marker, 1);
}

static gboolean
sigterm_iochn_data (GIOChannel *source, 
		    GIOCondition condition GNUC_UNUSED, 
		    gpointer user_data GNUC_UNUSED)
{
	GError *err = NULL;
	gchar data[1];
	gsize bytes_read;
	
	/* Empty the pipe */
	if (G_IO_STATUS_NORMAL != g_io_channel_read_chars (source, data, 1, &bytes_read, &err)) {
		warn ("Error emptying callout notify pipe: %s", err->message);
		g_error_free (err);
		goto out;
	}
	
	dbg ("Received SIGTERM, initiating shutdown\n");
	
	unmount_all ();
	
	gtk_main_quit ();
	
 out:
	return TRUE;
}


#ifdef ENABLE_NOTIFY
static void
statfs_mount_info_add (const char *udi)
{
	statfs_mount_info *info;
	
	if (g_hash_table_lookup (statfs_mounts, udi))
		return;
	
	if (libhal_device_get_property_bool (hal_ctx, udi, "volume.is_mounted_read_only", NULL))
		return;
	
	/* FIXME: this check can probably be dropped */
	if (gvm_udi_is_cdrom (udi))
		return;
	
	info = g_new0 (statfs_mount_info, 1);
	info->udi = g_strdup (udi);
	info->notified = FALSE;
	info->last_notified = 0.0;
	
	g_hash_table_insert (statfs_mounts, info->udi, info);
	
	gvm_statfs_check_space (info->udi, info, NULL);
}

static void
statfs_mount_info_remove (const char *udi)
{
	statfs_mount_info *info;
	
	if ((info = g_hash_table_lookup (statfs_mounts, udi))) {
		g_hash_table_remove (statfs_mounts, udi);
		statfs_mount_info_free (info);
	}
}

static void
statfs_mount_info_free (statfs_mount_info *info)
{
	g_free (info->udi);
	g_free (info);
}

static gboolean
gvm_statfs_check_space (const char *udi, statfs_mount_info *info, gpointer user_data GNUC_UNUSED)
{
	char *mount_point = NULL;
	NotifyNotification *n;
	struct statvfs buf;
	
	if (!(mount_point = libhal_device_get_property_string (hal_ctx, udi, "volume.mount_point", NULL)))
		return TRUE;
	
	/* Ignore volumes in /media */
	if (g_str_has_prefix (mount_point, "/media"))
		return TRUE;
	
	if (statvfs (mount_point, &buf) != -1) {
		unsigned long twogb_blocks;
		double free_space;
		
		free_space = (double) buf.f_bavail / (double) buf.f_blocks;
		twogb_blocks = (2 * 1024 * 1024) / buf.f_bsize;

		/*
		   If we continue using up more and more space or if we pass
		   the threshold value while running, we want to notify again
		*/
		if ((info->last_notified - free_space) > config.percent_used
		    || (free_space < config.percent_threshold && info->last_notified > config.percent_threshold))
			info->notified = FALSE;

		/*
		   If we didn't notify before (or need to again as above),
		   and the %free is less than the threshold, and the amount of
		   free space is less than 2GB, then notify the user
		*/
		if (!info->notified && (free_space < config.percent_threshold && buf.f_bavail < twogb_blocks)) {
			char *disk, *label, *msg, *icon;
			int in_use;
			
			label = libhal_device_get_property_string (hal_ctx, udi, "volume.label", NULL);
			if (!label || label[0] == '\0' || !strcmp (label, mount_point))
				disk = g_strdup (mount_point);
			else
				disk = g_strdup_printf ("%s (%s)", label, mount_point);
			libhal_free_string (label);
			
			in_use = 100 - (free_space * 100);
			
			if (!strcmp (disk, "/"))
				msg = g_strdup_printf (_("%d%% of the disk space on the root partition is in use"), in_use);
			else
				msg = g_strdup_printf (_("%d%% of the disk space on `%s' is in use"), in_use, disk);
			g_free (disk);
			
			icon = libhal_device_get_property_string (hal_ctx, udi, "info.icon_name", NULL);
			if (icon != NULL)
				n = notify_notification_new (_("Low Disk Space"), msg, icon, NULL);
			else
				n = notify_notification_new (_("Low Disk Space"), msg, "drive-harddisk", NULL);
			
			notify_notification_set_urgency (n, NOTIFY_URGENCY_CRITICAL);
			notify_notification_show (n, NULL);
			g_object_unref (n);
			g_free (msg);
			
			info->notified = TRUE;
			info->last_notified = free_space;
		}
	}
	
	libhal_free_string (mount_point);
	
	return TRUE;
}

static gboolean
gvm_statfs_timeout (gpointer user_data GNUC_UNUSED)
{
	g_hash_table_foreach (statfs_mounts, (GHFunc) gvm_statfs_check_space, NULL);
	
	return TRUE;
}
#endif /* ENABLE_NOTIFY */

static void
gvm_die (GnomeClient *client GNUC_UNUSED,
	 gpointer user_data GNUC_UNUSED)
{
	dbg ("Received 'die', initiating shutdown\n");
	
	unmount_all ();
	
	gtk_main_quit ();
}


#ifdef __linux__
enum {
	LOCAL_USER_CHECKED = (1 << 0),
	LOCAL_USER_FOUND   = (1 << 1)
};

/* checks that the user is logged-in at a local X session (which does not necessarily infer an *active* session) */
static gboolean
gvm_user_is_local_fallback (void)
{
	static guint local = 0;
	struct dirent *dent;
	struct utmp *utmp;
	const char *user;
	char *vtend;
	size_t n;
	DIR *dir;
	int vt;
	
	if (local & LOCAL_USER_CHECKED)
		return (local & LOCAL_USER_FOUND);
	
	user = g_get_user_name ();
	n = strlen (user);
	
	if (!(dir = opendir (GVM_CONSOLE_AUTH_DIR)))
		goto fallback;
	
	/* this works for pam_console ($path/user) and pam_foreground ($path/user:vt) - see bug #336932 */
	while ((dent = readdir (dir))) {
                if (!strncmp (user, dent->d_name, n)
		    && (dent->d_name[n] == '\0'
			|| (dent->d_name[n] == ':'
			    && ((vt = strtol (dent->d_name + n + 1, &vtend, 10)) >= 0)
			    && *vtend == '\0'))) {
			local = LOCAL_USER_FOUND;
			break;
		}
	}
	
	closedir (dir);
	
 fallback:
	
	if (!(local & LOCAL_USER_FOUND)) {
		setutent ();
		
		while (!(local & LOCAL_USER_FOUND) && (utmp = getutent ())) {
			if (utmp->ut_type != USER_PROCESS || strncmp (utmp->ut_user, user, n) != 0)
				continue;
			
			/* only accept local X sessions or local tty's (user started X via `startx`) */
			local = (utmp->ut_line[0] == ':' && utmp->ut_line[1] >= '0' && utmp->ut_line[1] <= '9')
				|| !strncmp (utmp->ut_line, "tty", 3) ? LOCAL_USER_FOUND : 0;
		}
		
		endutent ();
	}
	
	local |= LOCAL_USER_CHECKED;
	
	return (local & LOCAL_USER_FOUND);
}
#endif /* __linux__ */


static GPtrArray *
gvm_console_kit_get_seats (void)
{
	DBusMessage *dmesg, *reply;
	DBusMessageIter iter, elem;
	GPtrArray *seats = NULL;
	DBusError error;
	char *path;
	
	if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.ConsoleKit",
						    "/org/freedesktop/ConsoleKit/Manager",
						    "org.freedesktop.ConsoleKit.Manager",
						    "GetSeats"))) {
		dbg ("failed to create ConsoleKit.Manager request\n");
		return NULL;
	}
	
	dbus_error_init (&error);
	if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
		dbg ("ConsoleKit.GetSeats request failed to reply\n");
		dbus_message_unref (dmesg);
		dbus_error_free (&error);
		return NULL;
	}
	
	dbus_message_unref (dmesg);
	
	dbus_message_iter_init (reply, &iter);
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY ||
	    dbus_message_iter_get_element_type (&iter) != DBUS_TYPE_OBJECT_PATH) {
		dbg ("Unrecognized response to ConsoleKit.GetSeats request\n");
		dbus_message_unref (reply);
		return NULL;
	}
	
	dbus_message_iter_recurse (&iter, &elem);
	
	seats = g_ptr_array_new ();
	
	do {
		if (dbus_message_iter_get_arg_type (&elem) == DBUS_TYPE_OBJECT_PATH) {
			dbus_message_iter_get_basic (&elem, &path);
			g_ptr_array_add (seats, g_strdup (path));
		}
	} while (dbus_message_iter_next (&elem));
	
	dbus_message_unref (reply);
	
	return seats;
}

static GPtrArray *
gvm_console_kit_seat_get_sessions (const char *seat_id)
{
	DBusMessage *dmesg, *reply;
	DBusMessageIter iter, elem;
	GPtrArray *sessions = NULL;
	DBusError error;
	char *path;
	
	if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.ConsoleKit", seat_id,
						    "org.freedesktop.ConsoleKit.Seat",
						    "GetSessions"))) {
		dbg ("failed to create ConsoleKit GetSessions request\n");
		return NULL;
	}
	
	dbus_error_init (&error);
	if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
		dbg ("ConsoleKit GetSessions request failed to reply\n");
		dbus_message_unref (dmesg);
		dbus_error_free (&error);
		return NULL;
	}
	
	dbus_message_unref (dmesg);
	
	dbus_message_iter_init (reply, &iter);
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_ARRAY ||
	    dbus_message_iter_get_element_type (&iter) != DBUS_TYPE_OBJECT_PATH) {
		dbg ("Unrecognized response to ConsoleKit GetSessions request\n");
		dbus_message_unref (reply);
		return NULL;
	}
	
	dbus_message_iter_recurse (&iter, &elem);
	
	sessions = g_ptr_array_new ();
	
	do {
		if (dbus_message_iter_get_arg_type (&elem) == DBUS_TYPE_OBJECT_PATH) {
			dbus_message_iter_get_basic (&elem, &path);
			g_ptr_array_add (sessions, g_strdup (path));
		}
	} while (dbus_message_iter_next (&elem));
	
	dbus_message_unref (reply);
	
	return sessions;
}

static gboolean
gvm_console_kit_session_get_uid (const char *session_id, uid_t *uid)
{
	DBusMessage *dmesg, *reply;
	DBusMessageIter iter;
	DBusError error;
	
	if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.ConsoleKit", session_id,
						    "org.freedesktop.ConsoleKit.Session",
						    "GetUnixUser"))) {
		dbg ("failed to create ConsoleKit GetUnixUser request\n");
		return FALSE;
	}
	
	dbus_error_init (&error);
	if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
		dbg ("ConsoleKit GetUnixUser request failed to reply\n");
		dbus_message_unref (dmesg);
		dbus_error_free (&error);
		return FALSE;
	}
	
	dbus_message_unref (dmesg);
	
	dbus_message_iter_init (reply, &iter);
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_INT32) {
		dbg ("Unrecognized response to ConsoleKit GetUnixUser request\n");
		dbus_message_unref (reply);
		return FALSE;
	}
	
	dbus_message_iter_get_basic (&iter, uid);
	
	dbus_message_unref (reply);
	
	return TRUE;
}

static gboolean
gvm_console_kit_session_get_bool (const char *session_id, const char *method, gboolean *value)
{
	DBusMessage *dmesg, *reply;
	DBusMessageIter iter;
	DBusError error;
	
	if (!(dmesg = dbus_message_new_method_call ("org.freedesktop.ConsoleKit", session_id,
						    "org.freedesktop.ConsoleKit.Session",
						    method))) {
		dbg ("failed to create ConsoleKit %s request\n", method);
		return FALSE;
	}
	
	dbus_error_init (&error);
	if (!(reply = dbus_connection_send_with_reply_and_block (dbus_connection, dmesg, -1, &error))) {
		dbg ("ConsoleKit %s request failed to reply\n", method);
		dbus_message_unref (dmesg);
		dbus_error_free (&error);
		return FALSE;
	}
	
	dbus_message_unref (dmesg);
	
	dbus_message_iter_init (reply, &iter);
	if (dbus_message_iter_get_arg_type (&iter) != DBUS_TYPE_BOOLEAN) {
		dbg ("Unrecognized response to ConsoleKit %s request\n", method);
		dbus_message_unref (reply);
		return FALSE;
	}
	
	dbus_message_iter_get_basic (&iter, value);
	
	dbus_message_unref (reply);
	
	return TRUE;
}

#define QUERY_BOOL(session, method, rv) gvm_console_kit_session_get_bool (session, method, rv)

enum {
	USER_IS_LOCAL  = 1 << 0,
	USER_IS_ACTIVE = 1 << 1,
};

static int
gvm_query_console_kit (guint32 query, guint32 *result)
{
	GPtrArray *sessions, *seats = NULL;
	char *session_id, *seat_id;
	gboolean found = FALSE;
	gboolean rv;
	guint i, j;
	uid_t uid;
	
	*result = 0;
	
	if (!(seats = gvm_console_kit_get_seats ()))
		return -1;
	
	for (i = 0; i < seats->len && !found; i++) {
		seat_id = seats->pdata[i];
		
		sessions = gvm_console_kit_seat_get_sessions (seat_id);
		if (sessions != NULL) {
			for (j = 0; j < sessions->len && !found; j++) {
				session_id = sessions->pdata[j];
				
				if (gvm_console_kit_session_get_uid (session_id, &uid) && uid == getuid ()) {
					if ((query & USER_IS_ACTIVE) && QUERY_BOOL (session_id, "IsActive", &rv) && rv)
						*result |= USER_IS_ACTIVE;
					if ((query & USER_IS_LOCAL) && QUERY_BOOL (session_id, "IsLocal", &rv) && rv)
						*result |= USER_IS_LOCAL;
					found = TRUE;
				}
				
				g_free (session_id);
			}
			
			for ( ; j < sessions->len; j++)
				g_free (sessions->pdata[j]);
			
			g_ptr_array_free (sessions, TRUE);
		}
		
		g_free (seat_id);
	}
	
	for ( ; i < seats->len; i++)
		g_free (seats->pdata[i]);
	
	g_ptr_array_free (seats, TRUE);
	
	return 0;
}

static gboolean
gvm_user_is_local (void)
{
	guint32 result = 0;
	
	if (gvm_query_console_kit (USER_IS_LOCAL, &result) != -1)
		return (result & USER_IS_LOCAL);
	
	/* querying ConsoleKit failed, fall back to old behavior */
#ifdef __linux__	
	return gvm_user_is_local_fallback ();
#else
	return FALSE;
#endif
}

/* checks that the user is at the local active X session */
static gboolean
gvm_user_is_active (void)
{
#ifdef ENABLE_MULTIUSER
	guint32 result = 0;
	
	if (gvm_query_console_kit (USER_IS_ACTIVE, &result) != -1)
		return (result & USER_IS_ACTIVE);
	
	/* querying ConsoleKit failed, fall back to old behavior */
	
	return TRUE;
#else
	/* for the non-multiuser configuration, we assume we're always on the active console */
	return TRUE;
#endif
}


static gboolean print_version = FALSE;
static const char *daemon_arg = NULL;
static gboolean no_daemon = FALSE;
static gboolean secret_mode = FALSE;

static GOptionEntry options[] = {
	{ "version", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &print_version,
	  N_("Print version and exit"), NULL },
	{ "daemon", 'd', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_STRING, (char **) &daemon_arg,
	  N_("Run as a daemon"), "<yes|no>" },
	{ "no-daemon", 'n', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &no_daemon,
	  N_("Don't run as a daemon"), NULL },
	{ "secret-mode", 's', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_NONE, &secret_mode,
	  N_("Run in secret mode"), NULL },
	{ NULL, '\0', 0, 0, NULL, NULL, NULL }
};


int
main (int argc, char **argv)
{
	gboolean daemonize = TRUE;
	GnomeProgram *program;
	GnomeClient *client;
	GOptionContext *ctx;
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);
	
	ctx = g_option_context_new (PACKAGE);
	g_option_context_add_main_entries (ctx, options, NULL);
	
	program = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, argc, argv,
				      GNOME_PARAM_GOPTION_CONTEXT, ctx, GNOME_PARAM_NONE);
	
	if (print_version) {
		fprintf (stdout, "%s version %s\n", PACKAGE, VERSION);
		exit (0);
	}
	
	if (daemon_arg != NULL) {
		if (!strcmp (daemon_arg, "yes") || !strcmp (daemon_arg, "true"))
			daemonize = TRUE;
		else if (!strcmp (daemon_arg, "no") || !strcmp (daemon_arg, "false"))
			daemonize = FALSE;
		else
			fprintf (stdout, _("Unrecognized --daemon argument: %s\n"), daemon_arg);
	}
	
	if (no_daemon)
		daemonize = FALSE;
	
	if (secret_mode)
		fprintf (stdout, "Run silent, run deep.\n");
	
	if (daemonize && daemon (0, 0) < 0) {
		warn ("daemonizing failed: %s", g_strerror (errno));
		return 1;
	}
	
	client = gnome_master_client ();
	g_signal_connect (client, "die", G_CALLBACK (gvm_die), NULL);
	
	if (!(hal_ctx = gvm_hal_init ()))
		return 1;
	
	if (gvm_get_clipboard () && gvm_user_is_local ()) {
		gnome_client_set_restart_style (client, GNOME_RESTART_ANYWAY);
	} else {
		gnome_client_set_restart_style (client, GNOME_RESTART_NEVER);
		if (gvm_user_is_local ())
			warn ("already running");
		
		return 1;
	}
	
	gvm_init_config ();
	
	/* SIGTERM handling via pipes  */
	if (pipe (sigterm_unix_signal_pipe_fds) != 0) {
		warn ("Could not setup pipe, errno=%d", errno);
		return 1;
	}
	
	sigterm_iochn = g_io_channel_unix_new (sigterm_unix_signal_pipe_fds[0]);
	if (sigterm_iochn == NULL) {
		warn ("Could not create GIOChannel");
		return 1;
	}
	
	g_io_add_watch (sigterm_iochn, G_IO_IN, sigterm_iochn_data, NULL);
	signal (SIGTERM, handle_sigterm);
	
	device_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
					      (GDestroyNotify) libhal_free_string);
	mount_table = g_hash_table_new (g_str_hash, g_str_equal);
	dialogs = g_hash_table_new (g_str_hash, g_str_equal);
	
#ifdef ENABLE_NOTIFY
	/* Initialize the notification connection */
	notify_init ("GNOME Volume Manager");
	
	/* Initialize the statfs_mounts hash table */
	statfs_mounts = g_hash_table_new (g_str_hash, g_str_equal);
#endif

	mount_all (hal_ctx);
	
#ifdef ENABLE_NOTIFY
	/* Add a timeout for our statfs checking */
	statfs_id = g_timeout_add (15000, (GSourceFunc) gvm_statfs_timeout, NULL);
#endif
	
	gtk_main ();
	
#ifdef ENABLE_NOTIFY
	g_hash_table_destroy (statfs_mounts);
	g_source_remove (statfs_id);
	statfs_id = 0;
#endif
	
	g_hash_table_destroy (device_table);
	g_hash_table_destroy (mount_table);
	g_object_unref (program);
	
	return 0;
}
