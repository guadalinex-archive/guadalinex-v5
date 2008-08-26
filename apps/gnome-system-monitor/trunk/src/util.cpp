#include <config.h>

#include <glib/gi18n.h>
#include <glib/gstring.h>
#include <gtk/gtk.h>

#include <libgnomevfs/gnome-vfs-utils.h>
#include <glibtop/proctime.h>
#include <glibtop/procstate.h>
#include <unistd.h>

#include <stddef.h>
#include <cstring>

#include "util.h"
#include "procman.h"

extern "C" {
#include "e_date.h"
}


static const char*
format_process_state(guint state)
{
  const char *status;

  switch (state)
    {
    case GLIBTOP_PROCESS_RUNNING:
      status = _("Running");
      break;

    case GLIBTOP_PROCESS_STOPPED:
      status = _("Stopped");
      break;

    case GLIBTOP_PROCESS_ZOMBIE:
      status = _("Zombie");
      break;

    case GLIBTOP_PROCESS_UNINTERRUPTIBLE:
      status = _("Uninterruptible");
      break;

    default:
      status = _("Sleeping");
      break;
    }

  return status;
}



static char *
mnemonic_safe_process_name(const char *process_name)
{
	const char *p;
	GString *name;

	name = g_string_new ("");

	for(p = process_name; *p; ++p)
	{
		g_string_append_c (name, *p);

		if(*p == '_')
			g_string_append_c (name, '_');
	}

	return g_string_free (name, FALSE);
}



static inline unsigned divide(unsigned *q, unsigned *r, unsigned d)
{
	*q = *r / d;
	*r = *r % d;
	return *q != 0;
}


/*
 * @param d: duration in centiseconds
 * @type d: unsigned
 */
static char *
format_duration_for_display(unsigned centiseconds)
{
	unsigned weeks = 0, days = 0, hours = 0, minutes = 0, seconds = 0;

	(void)(divide(&seconds, &centiseconds, 100)
	       && divide(&minutes, &seconds, 60)
	       && divide(&hours, &minutes, 60)
	       && divide(&days, &hours, 24)
	       && divide(&weeks, &days, 7));

	if (weeks)
		/* xgettext: weeks, days */
		return g_strdup_printf(_("%uw%ud"), weeks, days);

	if (days)
		/* xgettext: days, hours (0 -> 23) */
		return g_strdup_printf(_("%ud%02uh"), days, hours);

	if (hours)
		/* xgettext: hours (0 -> 23), minutes, seconds */
		return g_strdup_printf(_("%u:%02u:%02u"), hours, minutes, seconds);

	/* xgettext: minutes, seconds, centiseconds */
	return g_strdup_printf(_("%u:%02u.%02u"), minutes, seconds, centiseconds);
}



GtkWidget*
procman_make_label_for_mmaps_or_ofiles(const char *format,
					     const char *process_name,
					     unsigned pid)
{
	GtkWidget *label;
	char *name, *title;

	name = mnemonic_safe_process_name (process_name);
	title = g_strdup_printf(format, name, pid);
	label = gtk_label_new_with_mnemonic (title);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0f, 0.5f);

	g_free (title);
	g_free (name);

	return label;
}



#define KIBIBYTE_FACTOR (1UL << 10)
#define MEBIBYTE_FACTOR (1UL << 20)
#define GIBIBYTE_FACTOR (1UL << 30)


/**
 * SI_gnome_vfs_format_file_size_for_display:
 * @size:
 * 
 * Formats the file size passed in @bytes in a way that is easy for
 * the user to read. Gives the size in bytes, kibibytes, mebibytes or
 * gibibytes, choosing whatever is appropriate.
 * 
 * Returns: a newly allocated string with the size ready to be shown.
 **/

gchar*
SI_gnome_vfs_format_file_size_for_display (GnomeVFSFileSize size, GnomeVFSFileSize max_size)
{
	if (max_size == 0)
		max_size = size;

	if (max_size < (GnomeVFSFileSize) KIBIBYTE_FACTOR) {
		return g_strdup_printf (dngettext(GETTEXT_PACKAGE, "%u byte", "%u bytes",(guint) size), (guint) size);
	} else {
		guint factor;
		const char* format;

		if (max_size < (GnomeVFSFileSize) MEBIBYTE_FACTOR) {
		  factor = KIBIBYTE_FACTOR;
		  format = N_("%.1f KiB");
		} else if (max_size < (GnomeVFSFileSize) GIBIBYTE_FACTOR) {
		  factor = MEBIBYTE_FACTOR;
		  format = N_("%.1f MiB");
		} else {
		  factor = GIBIBYTE_FACTOR;
		  format = N_("%.1f GiB");
		}

		return g_strdup_printf(_(format), size / (double)factor);
	}
}



gboolean
load_symbols(const char *module, ...)
{
	GModule *mod;
	gboolean found_all = TRUE;
	va_list args;

	mod = g_module_open(module, static_cast<GModuleFlags>(G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL));

	if (!mod)
		return FALSE;

	procman_debug("Found %s", module);

	va_start(args, module);

	while (1) {
		const char *name;
		void **symbol;

		name = va_arg(args, char*);

		if (!name)
			break;

		symbol = va_arg(args, void**);

		if (g_module_symbol(mod, name, symbol)) {
			procman_debug("Loaded %s from %s", name, module);
		}
		else {
			procman_debug("Could not load %s from %s", name, module);
			found_all = FALSE;
			break;
		}
	}

	va_end(args);


	if (found_all)
		g_module_make_resident(mod);
	else
		g_module_close(mod);

	return found_all;
}


static gboolean
is_debug_enabled(void)
{
	static gboolean init;
	static gboolean enabled;

	if (!init) {
		enabled = g_getenv("GNOME_SYSTEM_MONITOR_DEBUG") != NULL;
		init = TRUE;
	}

	return enabled;
}


static double get_relative_time(void)
{
	static unsigned long start_time;
	GTimeVal tv;

	if (G_UNLIKELY(!start_time)) {
		glibtop_proc_time buf;
		glibtop_get_proc_time(&buf, getpid());
		start_time = buf.start_time;
	}

	g_get_current_time(&tv);
	return (tv.tv_sec - start_time) + 1e-6 * tv.tv_usec;
}


void
procman_debug_real(const char *file, int line, const char *func,
		   const char *format, ...)
{
	va_list args;
	char *msg;

	if (G_LIKELY(!is_debug_enabled()))
		return;

	va_start(args, format);
	msg = g_strdup_vprintf(format, args);
	va_end(args);

	g_debug("[%.3f %s:%d %s] %s", get_relative_time(), file, line, func, msg);

	g_free(msg);
}



namespace procman
{
  void size_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
			   GtkTreeModel *model, GtkTreeIter *iter,
			   gpointer user_data)
  {
    const guint index = GPOINTER_TO_UINT(user_data);

    guint64 size;
    GValue value = { 0 };

    gtk_tree_model_get_value(model, iter, index, &value);

    switch (G_VALUE_TYPE(&value)) {
    case G_TYPE_ULONG:
      size = g_value_get_ulong(&value);
      break;

    case G_TYPE_UINT64:
      size = g_value_get_uint64(&value);
      break;

    default:
      g_assert_not_reached();
    }

    g_value_unset(&value);

    char *str = SI_gnome_vfs_format_file_size_for_display(size);
    g_object_set(renderer, "text", str, NULL);
    g_free(str);
  }


  /*
    Same as above but handles size == 0 as not available
   */
  void size_na_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
			      GtkTreeModel *model, GtkTreeIter *iter,
			      gpointer user_data)
  {
    const guint index = GPOINTER_TO_UINT(user_data);

    guint64 size;
    GValue value = { 0 };

    gtk_tree_model_get_value(model, iter, index, &value);

    switch (G_VALUE_TYPE(&value)) {
    case G_TYPE_ULONG:
      size = g_value_get_ulong(&value);
      break;

    case G_TYPE_UINT64:
      size = g_value_get_uint64(&value);
      break;

    default:
      g_assert_not_reached();
    }

    g_value_unset(&value);

    if (size == 0)
      g_object_set(renderer, "markup", _("<i>N/A</i>"), NULL);
    else {
      char *str = SI_gnome_vfs_format_file_size_for_display(size);
      g_object_set(renderer, "text", str, NULL);
      g_free(str);
    }

  }


  void duration_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
			       GtkTreeModel *model, GtkTreeIter *iter,
			       gpointer user_data)
  {
    const guint index = GPOINTER_TO_UINT(user_data);

    unsigned time;
    GValue value = { 0 };

    gtk_tree_model_get_value(model, iter, index, &value);

    switch (G_VALUE_TYPE(&value)) {
    case G_TYPE_ULONG:
      time = g_value_get_ulong(&value);
      break;

    case G_TYPE_UINT64:
      time = g_value_get_uint64(&value);
      break;

    default:
      g_assert_not_reached();
    }

    g_value_unset(&value);

    time = 100 * time / ProcData::get_instance()->frequency;
    char *str = format_duration_for_display(time);
    g_object_set(renderer, "text", str, NULL);
    g_free(str);
  }


  void time_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
			   GtkTreeModel *model, GtkTreeIter *iter,
			   gpointer user_data)
  {
    const guint index = GPOINTER_TO_UINT(user_data);

    time_t time;
    GValue value = { 0 };

    gtk_tree_model_get_value(model, iter, index, &value);

    switch (G_VALUE_TYPE(&value)) {
    case G_TYPE_ULONG:
      time = g_value_get_ulong(&value);
      break;

    default:
      g_assert_not_reached();
    }

    g_value_unset(&value);

    char *str = procman_format_date_for_display(time);
    g_object_set(renderer, "text", str, NULL);
    g_free(str);
  }

  void status_cell_data_func(GtkTreeViewColumn *, GtkCellRenderer *renderer,
			     GtkTreeModel *model, GtkTreeIter *iter,
			     gpointer user_data)
  {
    const guint index = GPOINTER_TO_UINT(user_data);

    guint state;
    GValue value = { 0 };

    gtk_tree_model_get_value(model, iter, index, &value);

    switch (G_VALUE_TYPE(&value)) {
    case G_TYPE_UINT:
      state = g_value_get_uint(&value);
      break;

    default:
      g_assert_not_reached();
    }

    g_value_unset(&value);

    const char *str = format_process_state(state);
    g_object_set(renderer, "text", str, NULL);
  }


  template<>
  void tree_store_update<const char>(GtkTreeModel* model, GtkTreeIter* iter, int column, const char* new_value)
  {
    char* current_value;

    gtk_tree_model_get(model, iter, column, &current_value, -1);

    if (!current_value or std::strcmp(current_value, new_value) != 0)
      gtk_tree_store_set(GTK_TREE_STORE(model), iter, column, new_value, -1);

    g_free(current_value);
  }




  std::string format_rate(GnomeVFSFileSize rate, GnomeVFSFileSize max_rate)
  {
    char* bytes = SI_gnome_vfs_format_file_size_for_display(rate, max_rate);
    // xgettext: rate, 10MiB/s
    return make_string(g_strdup_printf(_("%s/s"), bytes));
  }
}



