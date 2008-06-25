/*
 * gedit-TLoleo-plugin.h
 * 
 * Copyright (C) 2007 - Intelligent Dialogue Systems S.L.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include "gedit-TLoleo-plugin.h"
#include "gedit-TLoleo-plugin-defs.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gtk/gtk.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-window.h>

#define GEDIT_TLOLEO_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TLOLEO_PLUGIN, GeditTLoleoPluginPrivate))

struct _GeditTLoleoPluginPrivate
{
	GtkToolItem 		 *synthesize_button;
	GtkSeparatorToolItem *separator_tool_item;	
};

GEDIT_PLUGIN_REGISTER_TYPE(GeditTLoleoPlugin, gedit_TLoleo_plugin)

typedef struct
{
	GtkActionGroup *action_group;
	guint           ui_id;
} WindowData;

static void TLoleo_cb (GtkAction *action, GeditWindow *window);

static const GtkActionEntry action_entries[] =
{
	{ "TLoleo",
	  GTK_STOCK_MEDIA_PLAY,
	  N_("Synthesize"),
	  NULL,
	  N_("Synthesize selected text with festival"),
	  G_CALLBACK (TLoleo_cb) },
};

static gboolean check_festival(GString **error)
{
  gboolean 		ret 			= FALSE;
  gchar 		**std_output		= NULL;
  gchar 		**std_error		= NULL;

  std_output = (gchar **)g_malloc( 1 *sizeof(gchar*));
  std_error  = (gchar **)g_malloc( 1 * sizeof(gchar *));
  std_output[0] = std_error[0] = NULL;
  std_output[0] = (gchar*)malloc(DPKG_QUERY_LENGTH * sizeof(char));
  std_error[0] = (gchar*)malloc(DPKG_QUERY_LENGTH * sizeof(char));

  if( std_output 	== NULL  	|| 	std_error 	== NULL 	||
      std_output[0] 	== NULL 	|| 	std_error[0] 	== NULL)
  {
    g_string_assign(*error, _("Can't check Festival package. Memory allocation failed."));
  }
  else
  {
    if( g_spawn_command_line_sync(GEDIT_TLOLEO_PLUGIN_CHECK_FESTIVAL_COMMAND,
				     std_output,
				     std_error,
				     NULL,
				     NULL) )
    {
      if( std_output != NULL )
      {
        if( g_pattern_match_simple("*festival install ok installed*",std_output[0]) )
        {
          g_string_assign(*error, "");        
	  ret = TRUE;		     	
        }
        else
        {
	  g_string_assign(*error, _("Festival is not installed. You must install it to use this plugin."));
	  ret = FALSE;     	
        }
      }
      else
        g_string_assign(*error, _("Can't check Festival package. g_spawn_command_line_sync failed."));
    }  
    else
    {  	
      g_string_assign(*error, _("Can't check Festival package. dpkg-query failed."));
    }

    g_free(std_output[0]);
    g_free(std_error[0]);
    g_free(std_output);
    g_free(std_error);    
  }

  return ret;
}

static gboolean check_hispavoices(GString **error)
{
  gboolean 		ret 			= FALSE;
  gchar 		**std_output		= NULL;
  gchar 		**std_error		= NULL;

  std_output = (gchar **)g_malloc( 1 *sizeof(gchar*));
  std_error  = (gchar **)g_malloc( 1 * sizeof(gchar *));
  std_output[0] = std_error[0] = NULL;
  std_output[0] = (gchar*)malloc(DPKG_QUERY_LENGTH * sizeof(char));
  std_error[0] = (gchar*)malloc(DPKG_QUERY_LENGTH * sizeof(char));

  if( std_output 	== NULL  	|| 	std_error 	== NULL 	||
      std_output[0] 	== NULL 	|| 	std_error[0] 	== NULL)
  {
    g_string_assign(*error, _("Can't check Hispavoces package. Memory allocation failed."));
  }
  else
  {
    if( g_spawn_command_line_sync(GEDIT_TLOLEO_PLUGIN_CHECK_VOICES_COMMAND,
	  		          std_output,
				  std_error,
				  NULL,
				  NULL) )
    {
      if( std_output != NULL )
      {
        if( g_pattern_match_simple("*festvox-palpc16k install ok installed*",std_output[0]) &&
      	    g_pattern_match_simple("*festvox-sflpc16k install ok installed*",std_output[0]) )
        {
 	   g_string_assign(*error, "");
      	   ret = TRUE;
        }
        else
        {
  	   g_string_assign(*error, _("Hispavoces are not installed. You must install both festvox-palpc16k and festvox-sflpc16k voices."));     	
           ret = FALSE;
        }
      }
      else
        g_string_assign(*error, _("Can't check Hispavoces package. g_spawn_command_line_sync failed."));
    }  
    else
    {  	
      g_string_assign(*error, _("Can't check Hispavoces package. dpkg-query failed."));
    }

    g_free(std_output[0]);
    g_free(std_error[0]);
    g_free(std_output);
    g_free(std_error);
  }

  return ret;
}

static gboolean components_checking(GeditWindow *window, GString **error)
{
  GDir *d		  = NULL;
		
  d = g_dir_open (GEDIT_TLOLEO_PLUGIN_TMP_DIR, 0, NULL);
  if (!d)
    g_mkdir(GEDIT_TLOLEO_PLUGIN_TMP_DIR, S_IRWXU);

  if ( !check_festival(error) )
  {
    gedit_warning (GTK_WINDOW (window), (*error)->str);		
  	return FALSE;
  }

  if( !check_hispavoices(error) )
  {
    gedit_warning (GTK_WINDOW (window), (*error)->str );		
	return FALSE;
  }

  return TRUE;
}

static gchar* format_text(gchar *plain_text)
{
  gint	count	= 0;
  int j = 0;
  int i = 0;
  gchar 	*tmp_buffer = NULL;
  gulong 	len	= 0;

  /* count " characters */  
  for( i = 0; plain_text[i] != '\0'; i++)
  {
    if( plain_text[i] == '\"' )
    {
	count++;
    }
  }
  
  len = (gulong) (sizeof(gchar) * (strlen(plain_text) + (count * 2) + 1));
  tmp_buffer = (gchar *)g_malloc( len );
  g_return_val_if_fail (tmp_buffer != NULL, NULL);

  /* add scape character \ to " character */  
  for(i = 0; plain_text[i] != '\0' ; i++)
  {    
    if( plain_text[i] == '\"' )
    {
      tmp_buffer[j] = '\\';
      j++;
    }

    tmp_buffer[j] = plain_text[i];
    j++;
  }

  tmp_buffer[j] = '\0';  

  return tmp_buffer;
}


static gboolean read_selected_text_as_isolatin(GeditWindow *window, gchar **text,GString **error)
{  
  GtkTextIter  			start_selection;
  GtkTextIter  			end_selection;
  gsize 				len;  
  gchar 				*plain_selected_text_UTF8	= NULL;  
  gchar 				*format_selected_text_UTF8	= NULL;  
  GeditView     		*active_view		= NULL;
  GtkTextBuffer 		*buffer				= NULL;
  const GeditEncoding 	*to_codeset 		= NULL;
  
  active_view = gedit_window_get_active_view (window);
  g_return_val_if_fail (active_view, FALSE);

  buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (active_view));
  g_return_val_if_fail (buffer != NULL, FALSE);

  if( gtk_text_buffer_get_selection_bounds(buffer, &start_selection, &end_selection) )
  {	
  	plain_selected_text_UTF8 = gtk_text_buffer_get_text(buffer, &start_selection, &end_selection,FALSE);  	

	format_selected_text_UTF8 = format_text(plain_selected_text_UTF8);
	g_return_val_if_fail (format_selected_text_UTF8 != NULL, FALSE);

  	len = strlen(format_selected_text_UTF8);
	to_codeset = gedit_encoding_get_from_charset ("ISO-8859-1");
	(*text) = g_convert (format_selected_text_UTF8,
						 len, 
						 "ISO-8859-1",
						 "UTF-8",
						 NULL,
						 NULL,
						 NULL);
	g_free (plain_selected_text_UTF8);
	g_free (format_selected_text_UTF8);
	
	if ((*text) == NULL)
      return FALSE;
	else		    
      return TRUE;    	
  }
  else
  {
	g_string_assign(*error, _("No text has been selected"));
	gedit_warning (GTK_WINDOW (window), (*error)->str );
	return FALSE;    
  }
  	
}

static void synthesize_isolatin_text(gchar *text, GString **error)
{
  gchar *command		= NULL;  
  gchar *script_content	= NULL;
  FILE  *fd				= NULL;
    
  /* create temporal script to launch festival in batch mode */
  fd = fopen(GEDIT_TLOLEO_PLUGIN_FESTIVAL_SCRIPT_FILE, "w+");
  if( fd != NULL)
  {  	
    script_content = g_strdup ("(voice_JuntaDeAndalucia_es_pa_diphone) ;; select Pedro Alonso voice\n");
    script_content = g_strconcat (script_content,
  							    "(SayText \"",
  							    text,
  							    "\")\n",
  							    "(exit)\n",
  							     NULL);
  							     
    fwrite(script_content, sizeof(gchar), strlen(script_content), fd);
	fclose(fd);    

    /* synthesize text with festival in batch mode */
    command = g_strdup (GEDIT_TLOLEO_PLUGIN_FESTIVAL_BATCH_COMMAND);    
    g_spawn_command_line_async(command, NULL);
            
    g_free(command);
    g_free(script_content);    
  }
  else
  {
    g_string_assign(*error, _("Temporal script cannot be created"));  	
  }
}

static gboolean checking_audio_device(GeditWindow *window, GString **error)
{
  FILE *fd = NULL;  
  
  /* open sound device */  
  fd = fopen("/dev/dsp", "w");
  
  if (fd == NULL )
  {	  	
	g_string_assign(*error, _("Can't open sound device (\"/dev/dsp failed\")."));
	gedit_warning (GTK_WINDOW (window), (*error)->str );
    return FALSE;
  }	
  else
  {
    fclose(fd);
    return TRUE;
  }  
}

static void
gedit_TLoleo_plugin_init (GeditTLoleoPlugin *plugin)
{
	plugin->priv = GEDIT_TLOLEO_PLUGIN_GET_PRIVATE (plugin);	
	
	plugin->priv->separator_tool_item = NULL;
	plugin->priv->synthesize_button = NULL;

	gedit_debug_message (DEBUG_PLUGINS, "GeditTLoleoPlugin initialising");
}

static void
gedit_TLoleo_plugin_finalize (GObject *object)
{
/*
	GeditSamplePlugin *plugin = GEDIT_SAMPLE_PLUGIN (object);
*/
	gedit_debug_message (DEBUG_PLUGINS, "GeditTLoleoPlugin finalising");

	G_OBJECT_CLASS (gedit_TLoleo_plugin_parent_class)->finalize (object);
}

static void 
TLoleo_cb (GtkAction   *action,
	   GeditWindow *window)
{
	gboolean synth_process_is_ok;
	GString *error;			
	gchar *text = NULL;

	gedit_debug (DEBUG_PLUGINS);	

	/* init vbles */
	error = g_string_new("");
	synth_process_is_ok = TRUE;	

	/* checking components to synthesize */
	synth_process_is_ok = components_checking(window, &error);
	g_return_if_fail (synth_process_is_ok);

	/* checking sound device */ 	
	synth_process_is_ok = checking_audio_device(window, &error);
	g_return_if_fail (synth_process_is_ok);

	/* reading selected text */
	synth_process_is_ok =  read_selected_text_as_isolatin(window, &text, &error);
	g_return_if_fail (synth_process_is_ok);			

	/* synthesize the selected text */
	synthesize_isolatin_text(text, &error);
	g_return_if_fail (synth_process_is_ok);
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);
	g_free (data);
}

static void
update_ui_real (GeditWindow  *window,
		WindowData   *data)
{
	GeditView *view;
	GtkAction *action;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);

	gedit_debug_message (DEBUG_PLUGINS, "View: %p", view);
	
	action = gtk_action_group_get_action (data->action_group,
					      "TLoleo");
	gtk_action_set_sensitive (action,
				  (view != NULL) &&
				  gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));  
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	GtkToolItem *synthesize_button;
	GtkSeparatorToolItem *separator_tool_item;
	GtkWidget   *toolbar;
	GtkAction   *action;
	GeditTLoleoPlugin *TLoleo_plugin;		

	gedit_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);

	manager = gedit_window_get_ui_manager (window);
	
	data->action_group = gtk_action_group_new ("GeditTLoleoPluginActions");
	gtk_action_group_set_translation_domain (data->action_group, 
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (data->action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries), 
				      window);

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_new_merge_id (manager);

	g_object_set_data_full (G_OBJECT (window), 
				WINDOW_DATA_KEY, 
				data,
				(GDestroyNotify) free_window_data);

	gtk_ui_manager_add_ui (manager, 
			       data->ui_id, 
			       MENU_PATH,
			       "synthesizer", 
			       "TLoleo",
			       GTK_UI_MANAGER_MENUITEM, 
			       TRUE);
	
	/* add separator tool item to the toolbar */
	toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
 	TLoleo_plugin = GEDIT_TLOLEO_PLUGIN (plugin);	
	
	separator_tool_item = GTK_SEPARATOR_TOOL_ITEM(gtk_separator_tool_item_new());
	TLoleo_plugin->priv->separator_tool_item = separator_tool_item;
	
	gtk_widget_show (GTK_WIDGET (separator_tool_item));	
	
	gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
			    GTK_TOOL_ITEM(separator_tool_item),
			    -1);
	
	/* add synthesize button to the toolbar */	
	synthesize_button = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	TLoleo_plugin->priv->synthesize_button = synthesize_button;
				   
	action = gtk_action_group_get_action (data->action_group,
					      "TLoleo");			   
	g_object_set (action,
		      "is_important", TRUE,		      
		      NULL);
	gtk_action_connect_proxy (action, GTK_WIDGET (synthesize_button));

	gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
			    synthesize_button,
			    -1);
			    
    gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (synthesize_button), FALSE);

	update_ui_real (window, data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;	
	GeditTLoleoPlugin *TLoleo_plugin;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
	
	/* Remove separator tool item and TLoleo tool button */
	TLoleo_plugin = GEDIT_TLOLEO_PLUGIN (plugin);
	gtk_widget_destroy(GTK_WIDGET(TLoleo_plugin->priv->synthesize_button));
	gtk_widget_destroy(GTK_WIDGET(TLoleo_plugin->priv->separator_tool_item));
	
	/* Remove tmp dir and files */	
	g_unlink(GEDIT_TLOLEO_PLUGIN_FESTIVAL_SCRIPT_FILE);
	g_rmdir(GEDIT_TLOLEO_PLUGIN_TMP_DIR);		
}
		 
static void
impl_update_ui	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	WindowData   *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	update_ui_real (window, data);
}

static void
gedit_TLoleo_plugin_class_init (GeditTLoleoPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_TLoleo_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
	
	g_type_class_add_private (object_class, sizeof (GeditTLoleoPluginPrivate));
}
