/*
 * Hide Inactive buddies plugin.
 *
 * Copyright (C) 2008 Sumit Kumar Agrawal <talktosumit@gmail.com>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111-1301, USA.
 *
 */

/**
 * Author: Sumit Kumar Agrwal
 * Version: 1.0 
 */

#include "internal.h"
#include "pidgin.h"
#include "gtkplugin.h"
#include "gtkutils.h"
#include "gtkblist.h"
#include "prefs.h"
#include "version.h"
#include "debug.h"

#define HIDE_INACTIVE_PLUGIN_ID "gtk-hide-inactive-plugin"
enum
{
	PURPLE_STATUS_IDLE = 100,
} PurpleMissingStatus;

static struct PurpleContactInactiveStatuses
{
	int id;
	const char *name;
	const char *description;
} const statuses[] =
{
	{ PURPLE_STATUS_UNAVAILABLE,		"unavailable",		N_("Do not disturb")		},
	{ PURPLE_STATUS_INVISIBLE,			"invisible",		N_("Invisible")				},
	{ PURPLE_STATUS_AWAY,				"away",				N_("Away")					},
	{ PURPLE_STATUS_EXTENDED_AWAY,		"extended_away",	N_("Extended away")			},
	{ PURPLE_STATUS_MOBILE,				"mobile",			N_("Mobile")				},
	{ PURPLE_STATUS_TUNE,				"tune",				N_("Listening to music")	},
	{ PURPLE_STATUS_IDLE,				"idle",				N_("Idle")					},
	{ PURPLE_STATUS_UNSET,				NULL,				NULL						}
};

static gboolean 
buddy_primitive_is_displayable(const PurpleBuddy * buddy, PurpleStatusPrimitive primitive)
{
	PurplePresence * presence = buddy->presence;
	gboolean ret;
	ret = ((purple_presence_is_online(presence) && 
	  (purple_blist_node_get_bool((PurpleBlistNode*)buddy, "show_inactive") ||
				 !((purple_prefs_get_bool("/purple/status/inactive/unavailable") && primitive == PURPLE_STATUS_UNAVAILABLE) ||
					 (purple_prefs_get_bool("/purple/status/inactive/invisible") && primitive == PURPLE_STATUS_INVISIBLE) ||
					 (purple_prefs_get_bool("/purple/status/inactive/idle") && purple_presence_is_idle(presence)) || 
					 (purple_prefs_get_bool("/purple/status/inactive/away") && primitive == PURPLE_STATUS_AWAY) || 
					 (purple_prefs_get_bool("/purple/status/inactive/extended_away") && primitive == PURPLE_STATUS_EXTENDED_AWAY) ||
					 (purple_prefs_get_bool("/purple/status/inactive/tune") && purple_presence_is_status_primitive_active(presence, PURPLE_STATUS_TUNE)) ||
					 (purple_prefs_get_bool("/purple/status/inactive/mobile") && purple_presence_is_status_primitive_active(presence, PURPLE_STATUS_MOBILE))))) || (!purple_presence_is_online(presence) && ! purple_prefs_get_bool("/purple/status/inactive/offline")));
	return ret;
}

static gboolean 
buddy_is_displayable(const PurpleBuddy * buddy)
{
	PurpleStatus *status;
	PurpleStatusPrimitive primitive;
	PurpleStatusType* status_type;
	
	PurplePresence * presence = buddy->presence;
	g_return_val_if_fail(presence != NULL, FALSE);

	status = purple_presence_get_active_status(presence);
	g_return_val_if_fail(status != NULL, FALSE);

	status_type = purple_status_get_type(status);
	g_return_val_if_fail(status_type != NULL, FALSE);

	primitive = purple_status_type_get_primitive(status_type);

	return buddy_primitive_is_displayable(buddy, primitive);
}	

static void 
buddy_status_changed(PurpleBuddy *buddy, gboolean should_show)
{
	if(should_show == TRUE)
		purple_blist_node_set_flags(((PurpleBlistNode*)buddy), 
									(purple_blist_node_get_flags(((PurpleBlistNode*)buddy)) & ~PURPLE_BLIST_NODE_FLAG_NO_DISPLAY));
	else
		purple_blist_node_set_flags(((PurpleBlistNode*)buddy), 
									(purple_blist_node_get_flags(((PurpleBlistNode*)buddy)) | PURPLE_BLIST_NODE_FLAG_NO_DISPLAY));
}

static void
update_buddy(PurpleBuddy *buddy, PurpleStatusPrimitive primitive_selected)
{
	gboolean should_show = TRUE;
	PurpleStatus *status;
	PurpleStatusPrimitive primitive;
	PurpleStatusType* status_type;
	
	PurplePresence * presence = buddy->presence;
	g_return_if_fail(presence != NULL);

	status = purple_presence_get_active_status(presence);
	g_return_if_fail(status != NULL);

	status_type = purple_status_get_type(status);
	g_return_if_fail(status_type != NULL);

	primitive = purple_status_type_get_primitive(status_type);
	
	if((primitive_selected == primitive) ||
			(primitive_selected == PURPLE_STATUS_MOBILE && 
				purple_presence_is_status_primitive_active(presence, PURPLE_STATUS_MOBILE)) ||
			(primitive_selected == PURPLE_STATUS_TUNE && 
				purple_presence_is_status_primitive_active(presence, PURPLE_STATUS_MOBILE)) ||
			(primitive_selected == PURPLE_STATUS_IDLE && 
				purple_presence_is_idle(presence)))
	{
		should_show = buddy_primitive_is_displayable(buddy, primitive);
		buddy_status_changed(buddy, should_show);
	}
}

static void 
buddy_signonoff_cb(PurpleBuddy *buddy)
{
	buddy_status_changed(buddy, buddy_is_displayable(buddy));
	pidgin_blist_refresh(purple_get_blist());
}

static void
buddy_status_changed_cb(PurpleBuddy *buddy, PurpleStatus *old_status, PurpleStatus *status)
{
	buddy_status_changed(buddy, buddy_is_displayable(buddy));
	pidgin_blist_refresh(purple_get_blist());
}

static void
buddy_idle_changed_cb(PurpleBuddy *buddy, gboolean old_idle, gboolean idle)
{ 
	buddy_status_changed(buddy, buddy_is_displayable(buddy));
	pidgin_blist_refresh(purple_get_blist());
}

static void
pref_update(GtkWidget *widget, int i)
{
	PurpleBlistNode *node;
	PurpleBuddy * buddy;
	
	char *pref = g_strconcat("/purple/status/inactive/", statuses[i].name, NULL);
	gboolean pref_value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	purple_prefs_set_bool(pref, pref_value);
	node = purple_get_blist()->root;

	while(node)
	{
		if(PURPLE_BLIST_NODE_IS_BUDDY(node))
		{
			buddy = (PurpleBuddy *) node;
			update_buddy(buddy, statuses[i].id);
		}
		node = purple_blist_node_next(node, FALSE);
	}
	pidgin_blist_refresh(purple_get_blist());
}

static GtkWidget *
get_config_frame(PurplePlugin *plugin)
{
	GtkWidget *ret = NULL, *hbox = NULL, *frame = NULL, *vbox = NULL;
	GtkWidget *check = NULL;
	GtkSizeGroup *sg = NULL;
	int i;

	sg = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	ret = gtk_vbox_new(FALSE, 18);
	gtk_container_set_border_width(GTK_CONTAINER(ret), 12);

	frame = pidgin_make_frame(ret, _("Hide buddy when status is ..."));

	vbox = gtk_vbox_new(FALSE, 5);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	for (i = 0 ; statuses[i].name != NULL && statuses[i].description != NULL ; i++)
	{
		char *pref = g_strconcat("/purple/status/inactive/", statuses[i].name, NULL);
		gboolean value = purple_prefs_get_bool(pref);

		hbox = gtk_hbox_new(FALSE, 5);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

		check = gtk_check_button_new_with_label(_(statuses[i].description));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), value);
		g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_update), i);
		gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);

		g_free(pref);
	}

	gtk_widget_show_all(ret);
	g_object_unref(sg);

	return ret;
}

static gboolean
plugin_load(PurplePlugin *plugin)
{
	PurpleBlistNode *node;
	PurpleBuddy * buddy;
	
	if(!purple_prefs_exists("/purple/status"))
		purple_prefs_add_none("/purple/status");
	if(!purple_prefs_exists("/purple/status/inactive"))
		purple_prefs_add_none("/purple/status/inactive");
	
	node = purple_get_blist()->root;
	while(node)
	{
		if(PURPLE_BLIST_NODE_IS_BUDDY(node))
		{
			buddy = (PurpleBuddy *) node;
			buddy_status_changed(buddy, buddy_is_displayable(buddy));
		}
		node = purple_blist_node_next(node, FALSE);
	}
	
	pidgin_blist_refresh(purple_get_blist());
    
	purple_signal_connect(purple_blist_get_handle(), "buddy-signed-on", plugin, PURPLE_CALLBACK(buddy_signonoff_cb), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-signed-off", plugin, PURPLE_CALLBACK(buddy_signonoff_cb), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-status-changed", plugin, PURPLE_CALLBACK(buddy_status_changed_cb), NULL);
	purple_signal_connect(purple_blist_get_handle(), "buddy-idle-changed", plugin, PURPLE_CALLBACK(buddy_idle_changed_cb), NULL);
	    
	return TRUE;
}

static gboolean
plugin_unload(PurplePlugin *plugin)
{
	PurpleBlistNode *node;

	node = purple_get_blist()->root;
	while(node)
	{
		if(PURPLE_BLIST_NODE_IS_BUDDY(node))
		{
			purple_blist_node_set_flags(node, (purple_blist_node_get_flags(node)  & ~PURPLE_BLIST_NODE_FLAG_NO_DISPLAY));
		}
		node = purple_blist_node_next(node, FALSE);
	}
	
	pidgin_blist_refresh(purple_get_blist());

	return TRUE;
}

static PidginPluginUiInfo ui_info =
{
	get_config_frame,
	0, /* page_num (Reserved) */
	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static PurplePluginInfo info =
{
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,                             /**< type           */
	PIDGIN_PLUGIN_TYPE,                             /**< ui_requirement */
	0,                                                /**< flags          */
	NULL,                                             /**< dependencies   */
	PURPLE_PRIORITY_DEFAULT,                            /**< priority       */

	HIDE_INACTIVE_PLUGIN_ID,                       /**< id             */
	N_("Hide inactive buddies"),                           /**< name           */
	"1.0",                                  /**< version        */
	/**< summary        */
	N_("Hide inactive buddies. Determine inactive buddies by status."),
	/**< description    */
	N_("Hide inactive buddies from buddy list. Determine inactive buddies by status type.\n"), 
	"Sumit Kumar Agrawal <talktosumit@gmail.com>",         /**< author         */
	PURPLE_WEBSITE,                                     /**< homepage       */

	plugin_load,                                             /**< load           */
	plugin_unload,                                             /**< unload         */
	NULL,                                             /**< destroy        */
	&ui_info,                                         /**< ui_info        */
	NULL,                                             /**< extra_info     */
	NULL,                                             /**< prefs_info     */
	NULL,                                             /**< actions        */

	/* padding */
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin(PurplePlugin *plugin)
{
}

PURPLE_INIT_PLUGIN(contactpriority, init_plugin, info)
