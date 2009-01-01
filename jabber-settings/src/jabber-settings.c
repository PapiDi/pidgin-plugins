/*
 * Plugin to configure extra jabber (gmail) settings
 *
 * Copyright (C) 2008-11 Sumit Agrawal <talktosumit@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifndef _WIN32
# include <X11/Xlib.h>
#include <unistd.h>
#endif

/* config.h may define PURPLE_PLUGINS; protect the definition here so that we
 * don't get complaints about redefinition when it's not necessary. */
#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#include <glib.h>
/* This will prevent compiler errors in some instances and is better explained in the
 * how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

/* System headers */
#include<string.h>

/* Purple headers */
#include <version.h>
#include <notify.h>
#include <plugin.h>
#include <debug.h>
#include<cmds.h>
#include<blist.h>

PurplePlugin *jabber_setting_plugin = NULL;

static void google_otr_received(PurpleAccount *account, const char *who, const char *val);

static void xmlnode_sending_cb(PurpleConnection *gc, xmlnode ** packet, gpointer null)
{
	xmlnode *child;
	if(*packet != NULL) {
		if(!strcmp((*packet)->name, "message")) {
			for(child = (*packet)->child; child; child = child->next) {
				if(!strcmp(child->name, "gone") ) 
				{
					if(purple_prefs_get_bool("/plugins/core/jabbersetting/block_notification") == TRUE)
					{
						*packet = NULL, packet = NULL;
					}
				}
			}
		}
	}
}

static void xmlnode_receive_cb(PurpleConnection *gc, xmlnode ** packet, gpointer null)
{
	xmlnode *child;
	char *from;
	const char *nosvalue, *xmlns;

	if(*packet != NULL) {
		if(!strcmp((*packet)->name, "message")) {
			from = g_strdup(xmlnode_get_attrib(*packet, "from"));
			for(child = (*packet)->child; child; child = child->next) {
				xmlns = xmlnode_get_namespace(child);
				if(!strcmp(child->name, "x") && !strcmp(xmlns, "google:nosave")) {
					nosvalue = xmlnode_get_attrib(child, "value");
					if(nosvalue)
						google_otr_received(gc->account, from, nosvalue);
				}
			}
		}
	}
}

static void conversation_created_cb(PurpleConversation *conv)
{
	const char *name = purple_conversation_get_name(conv);
	PurpleAccount * account = purple_conversation_get_account(conv);

	if (purple_conversation_get_type(conv) == PURPLE_CONV_TYPE_IM && strstr(conv->account->protocol_id, "prpl-jabber") != NULL)
	{
		GSList *buddies;
		GSList *cur;

		buddies = purple_find_buddies(account, name);
		for (cur = buddies; cur != NULL; cur = cur->next)
		{
			PurpleBlistNode *node = cur->data;
			const char * isenabled = purple_blist_node_get_string(node, "google-nosave");
			const char * isdisplayed = purple_blist_node_get_string(node, "google-nosave-displayed");
			if(isdisplayed && strncmp(isdisplayed, "false", 5) == 0)
			{
				if(!isenabled || strcmp(isenabled, "disabled") == 0)
				{
					if(purple_prefs_get_bool("/plugins/core/jabbersetting/show_otr"))
					{
						purple_conv_present_error(purple_conversation_get_name(conv), account, "This chat is no longer off the record"); //sumit remove this 
						purple_blist_node_set_string(node, "google-nosave-displayed", "true");
					}
				}
				else if(!isenabled || strcmp(isenabled, "enabled") == 0)
				{
					if(purple_prefs_get_bool("/plugins/core/jabbersetting/show_otr"))
					{
						purple_conv_present_error(purple_conversation_get_name(conv), account, "This chat in now on off the record as requested from your friend. (from now on, chats with this buddy will not be saved in his gmail account or yours)");  
						purple_blist_node_set_string(node, "google-nosave-displayed", "true");
					}
				}
			}
		}
	}

}

static void google_otr_received(PurpleAccount *account, const char *who, const char *val)
{
	PurpleConversation *c = NULL;
	PurpleBuddy *buddy = NULL;
	const char * isenabled;
	if(NULL == account) 
		return;

	if ( (buddy = purple_find_buddy(account, who)) == NULL)
		return;
	if(strstr(account->protocol_id, "prpl-jabber") != NULL)
	{
		isenabled = purple_blist_node_get_string((PurpleBlistNode *) buddy, "google-nosave");
		c = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, who, account);

		if(strcmp(val, "disabled") == 0 && (!isenabled || strcmp(isenabled, "enabled") == 0))
		{
			purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave", val);
			if(c && purple_prefs_get_bool("/plugins/core/jabbersetting/show_otr"))
			{
				purple_conv_present_error(purple_conversation_get_name(c), account, "This chat is no longer off the record"); //sumit remove this 
				purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave-displayed", "true");
			}
			else
				purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave-displayed", "false");
		}
		else if(strcmp(val, "enabled") == 0 && (!isenabled || strcmp(isenabled, "disabled") == 0))
		{
			purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave", val);
			if(c && purple_prefs_get_bool("/plugins/core/jabbersetting/show_otr"))
			{
				purple_conv_present_error(purple_conversation_get_name(c), account, "This chat in now on off the record as requested from your friend. (from now on, chats with this buddy will not be saved in his gmail account or yours)");  
				purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave-displayed", "true");
			}
			else
				purple_blist_node_set_string((PurpleBlistNode *) buddy, "google-nosave-displayed", "false");
		}
	}
}

static PurplePluginPrefFrame *
get_plugin_pref_frame(PurplePlugin *plugin)
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *ppref;

	frame = purple_plugin_pref_frame_new();

	ppref = purple_plugin_pref_new_with_label("Settings");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/jabbersetting/block_notification", "Block \"left conversation\" notification");
	purple_plugin_pref_frame_add(frame, ppref);

	ppref = purple_plugin_pref_new_with_name_and_label("/plugins/core/jabbersetting/show_otr", "Show \"Off the record\" notification in conversation window");
	purple_plugin_pref_frame_add(frame, ppref);

	return frame;

}

static gboolean plugin_load (PurplePlugin * plugin)
{
	PurplePlugin *jabber;

	jabber_setting_plugin = plugin; /* assign this here so we have a valid handle later */

	purple_signal_connect(purple_conversations_get_handle(), "conversation-created", plugin, PURPLE_CALLBACK(conversation_created_cb), NULL);
	jabber = purple_find_prpl("prpl-jabber");
	if (jabber)
	{
		purple_signal_connect(jabber, "jabber-sending-xmlnode", plugin, PURPLE_CALLBACK(xmlnode_sending_cb), NULL);
		purple_signal_connect(jabber, "jabber-receiving-xmlnode", plugin, PURPLE_CALLBACK(xmlnode_receive_cb), NULL);
		//purple_signal_connect(purple_blist_get_handle(), "blist-node-extended-menu", plugin, PURPLE_CALLBACK(gtk_blist_jabber_setting_cb), NULL);
	}
	return TRUE;
}

static gboolean plugin_unload (PurplePlugin * plugin)
{
	PurplePlugin *jabber;

	jabber = purple_find_prpl("prpl-jabber");
	if (jabber)
	{
		purple_signal_disconnect(jabber, "jabber-sending-xmlnode", plugin, PURPLE_CALLBACK(xmlnode_sending_cb));
		purple_signal_disconnect(jabber, "jabber-receiving-xmlnode", plugin, PURPLE_CALLBACK(xmlnode_receive_cb));
		//purple_signal_disconnect(purple_blist_get_handle(), "blist-node-extended-menu", plugin, PURPLE_CALLBACK(gtk_blist_jabber_setting_cb));
	}
	return TRUE;
}

static PurplePluginUiInfo ui_info =
{
	get_plugin_pref_frame,
	0, /* page_num (Reserved) */
	NULL,

	/* Padding */
	NULL,
	NULL,
	NULL,
	NULL
};

/* For specific notes on the meanings of each of these members, consult the C Plugin Howto
 * on the website. */
static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"jabber-settings",
	"Jabber settings",
	"1.0", /* This constant is defined in config.h, but you shouldn't use it for
			  your own plugins.  We use it here because it's our plugin. And we're lazy. */

	"Plugin to configure extra jabber (gmail) settings",
	"Plugin to configure extra jabber (gmail) settings\n1. Notify if chat is off the record.\n2.Setting to block \"left conversation\" notification",
	"Sumit Agrawal <talktosumit@gmail.com>", /* correct author */
	"http://code.google.com/p/pidgin-plugins",
	plugin_load,
	plugin_unload,
	NULL,

	NULL,                                         /**< ui_info        */
	NULL,
	&ui_info,
	NULL,		/* this tells libpurple the address of the function to call
				   to get the list of plugin actions. */
	NULL,
	NULL,
	NULL,
	NULL
};

	static void
init_plugin (PurplePlugin * plugin)
{
	purple_prefs_add_none("/plugins/core/jabbersetting");
	purple_prefs_add_bool("/plugins/core/jabbersetting/block_notification", TRUE);
	purple_prefs_add_bool("/plugins/core/jabbersetting/show_otr", TRUE);
}

PURPLE_INIT_PLUGIN (hello_world, init_plugin, info)
