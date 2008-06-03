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
 
#ifndef __GEDIT_TLOLEO_PLUGIN_DEFS_H__
#define __GEDIT_TLOLEO_PLUGIN_DEFS_H__

G_BEGIN_DECLS

#define WINDOW_DATA_KEY "GeditTLoleoPluginWindowData"
#define MENU_PATH "/MenuBar/ToolsMenu/ToolsOps_2"

#define DPKG_QUERY_LENGTH 256

#define GEDIT_TLOLEO_PLUGIN_TMP_DIR "/tmp/gedit-2.festival/"
#define GEDIT_TLOLEO_PLUGIN_CHECK_FESTIVAL_COMMAND 		\
			"dpkg-query -W -f='${Package} ${Status}\n' festival"
#define GEDIT_TLOLEO_PLUGIN_CHECK_VOICES_COMMAND			\
			"dpkg-query -W -f='${Package} ${Status}\n' festvox*"
#define GEDIT_TLOLEO_PLUGIN_FESTIVAL_SCRIPT_FILE			\
			 GEDIT_TLOLEO_PLUGIN_TMP_DIR"script.scm"
#define GEDIT_TLOLEO_PLUGIN_FESTIVAL_BATCH_COMMAND			\
			"festival -b "GEDIT_TLOLEO_PLUGIN_FESTIVAL_SCRIPT_FILE

G_END_DECLS

#endif /* __GEDIT_TLOLEO_PLUGIN_DEFS_H__ */
