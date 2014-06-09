#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "internal.h"
#include "notify.h"
#include "plugin.h"
#include "request.h"
#include "version.h"
#include "debug.h"

#ifndef PURPLE_PLUGINS
#	define PURPLE_PLUGINS
#endif

#define PLUGIN_ID			"core-mixedcase-messages"
#define PLUGIN_NAME			"Mixed Case"
#define PLUGIN_STATIC_NAME	"mixedcase"
#define PLUGIN_AUTHOR		"David Carver <dcarver@mivamerchant.com>"
#define PLUGIN_SUMMARY		"Alternates letters between uppercase and lowercase in outgoing messages"
#define PLUGIN_DESCRIPTION	"Alternates letters between uppercase and lowercase in outgoing messages"
#define PLUGIN_PREFS		"/plugins/core/" PLUGIN_STATIC_NAME
#define PLUGIN_PREF_ON		PLUGIN_PREFS "/on"
#define PLUGIN_PREF_BLIST	PLUGIN_STATIC_NAME

typedef enum
{
	BLIST_DEFAULT,
	BLIST_ALWAYS,
	BLIST_NEVER
} MixedCaseBlistSettings;

static void flip_text( char **buffer )
{
	int i 				= 0;
	gboolean flip 		= TRUE;
	int buffer_length	= strlen( *buffer );
	
	if ( isupper( ( *buffer )[ 0 ] ) )
	{
		flip = !flip;
	}

	for( ; i < buffer_length; i++ )
	{
		if ( isalpha( ( *buffer )[ i ] ) )
		{
			( *buffer )[ i ] 	= ( flip ) ? toupper( ( *buffer )[ i ] ) : tolower( ( *buffer )[ i ] );
			flip				= !flip;
		}
	}
}

static void sending_im_msg_cb( PurpleAccount *account, char *recipient, char **buffer, void *data )
{
	int buddy_setting 		= -1;
	int plugin_setting 		= purple_prefs_get_int( PLUGIN_PREF_ON );
	PurpleBlistNode *node	= PURPLE_BLIST_NODE( purple_find_buddy( account, recipient ) );

	if ( g_strstr_len( *buffer, -1, "http://" ) != NULL || g_strstr_len( *buffer, -1, "https://" ) != NULL )
	{
		return;
	}

	if ( node )
	{
		node = purple_blist_node_get_parent( node );
		g_return_if_fail( PURPLE_BLIST_NODE_IS_CONTACT( node ) );
		buddy_setting = purple_blist_node_get_int( node, PLUGIN_PREF_BLIST );
	}

	if ( buddy_setting == BLIST_ALWAYS || ( plugin_setting && buddy_setting != BLIST_NEVER ) )
	{
		purple_debug_info( PLUGIN_STATIC_NAME, "Sent in mixed case\n" );
		flip_text( buffer );
	}
}

static void save_mixed_case_settings_cb( PurpleBlistNode *node, int choice )
{
	if ( PURPLE_BLIST_NODE_IS_BUDDY( node) )
	{
		node = purple_blist_node_get_parent( node );
	}

	g_return_if_fail( PURPLE_BLIST_NODE_IS_CONTACT( node ) );

	purple_blist_node_set_int( node, PLUGIN_PREF_BLIST, choice );
}

static void set_mixed_case_settings_cb( PurpleBlistNode *node, gpointer plugin )
{
	char *message;

	if ( PURPLE_BLIST_NODE_IS_BUDDY( node ) )
	{
		node = purple_blist_node_get_parent(node);
	}

	g_return_if_fail( PURPLE_BLIST_NODE_IS_CONTACT( node ) );

	message = g_strdup_printf( "When sending an instant message to %s", purple_contact_get_alias( ( PurpleContact * ) node ) );

	purple_request_choice( plugin,													/* Handle */
						   "Set Mixed Case Setting",								/* Title */
						   message,													/* Primary */
						   NULL,													/* Secondary */
						   purple_blist_node_get_int( node, PLUGIN_PREF_BLIST ),	/* Default value */
						   "Save",													/* OK text */
						   G_CALLBACK( save_mixed_case_settings_cb ),				/* OK callback */
						   "Cancel",												/* Cancel text */
						   NULL,													/* Cancel callback */
						   NULL,													/* Account */
						   NULL,													/* Who */
						   NULL,													/* Conversation */
						   node,													/* User data to pass to the callback */
						   "Use Default Settings",									/* First radio */
						   BLIST_DEFAULT,											/* Radio value */
						   "Always use mixed case",									/* Second radio */
						   BLIST_ALWAYS,											/* Radio value */
						   "Never use mixed case",									/* Third radio */
						   BLIST_NEVER,												/* Radio value */
						   NULL	);													/* NULL terminator for the choice */

	g_free( message );
}

static void blist_menu_cb( PurpleBlistNode *node, GList **menu, gpointer plugin )
{
	PurpleMenuAction *action;

	if ( !PURPLE_BLIST_NODE_IS_BUDDY( node ) && !PURPLE_BLIST_NODE_IS_CONTACT( node ) && !( purple_blist_node_get_flags( node ) & PURPLE_BLIST_NODE_FLAG_NO_SAVE ) )
	{
		return;
	}

	action 	= purple_menu_action_new( "Use Mixed Case...", PURPLE_CALLBACK( set_mixed_case_settings_cb ), plugin, NULL );
	*menu 	= g_list_prepend( *menu, action );
}

static gboolean plugin_load( PurplePlugin *plugin )
{
	purple_signal_connect( purple_conversations_get_handle(), 	"sending-im-msg", 			plugin, PURPLE_CALLBACK( sending_im_msg_cb ), 	plugin );
	purple_signal_connect( purple_blist_get_handle(), 			"blist-node-extended-menu", plugin, PURPLE_CALLBACK( blist_menu_cb ), 		plugin );

	return TRUE;
}

static gboolean plugin_unload( PurplePlugin *plugin )
{
	purple_signals_disconnect_by_handle( plugin );

	return TRUE;
}

static PurplePluginPrefFrame *get_plugin_pref_frame( PurplePlugin *plugin )
{
	PurplePluginPrefFrame *frame;
	PurplePluginPref *pref;

	frame 	= purple_plugin_pref_frame_new();
	pref 	= purple_plugin_pref_new_with_name_and_label( PLUGIN_PREF_ON, "Send instant messages using mixed case when:\n" );

	purple_plugin_pref_set_type( pref, PURPLE_PLUGIN_PREF_CHOICE );
	purple_plugin_pref_add_choice( pref, "Always",	GINT_TO_POINTER( 1 ) );
	purple_plugin_pref_add_choice( pref, "Never", 	GINT_TO_POINTER( 0 ) );
	purple_plugin_pref_frame_add( frame, pref );

	return frame;
}

static PurplePluginUiInfo prefs_info =
{
	get_plugin_pref_frame,
	0,   	/* page_num (Reserved) */
	NULL, 	/* frame (Reserved) */
	NULL, 	/* padding */
	NULL,	/* padding */
	NULL,	/* padding */
	NULL	/* padding */
};

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,		/* Magic				*/
	PURPLE_MAJOR_VERSION,		/* Purple Major Version	*/
	PURPLE_MINOR_VERSION,		/* Purple Minor Version	*/
	PURPLE_PLUGIN_STANDARD,		/* plugin type			*/
	NULL,						/* ui requirement		*/
	0,							/* flags				*/
	NULL,						/* dependencies			*/
	PURPLE_PRIORITY_DEFAULT,	/* priority				*/
	PLUGIN_ID,					/* plugin id			*/
	PLUGIN_NAME,				/* name					*/
	"1.00",						/* version				*/
	PLUGIN_SUMMARY,				/* summary				*/
	PLUGIN_DESCRIPTION,			/* description			*/
	PLUGIN_AUTHOR,				/* author				*/
	NULL,						/* website				*/
	plugin_load,				/* load					*/
	plugin_unload,				/* unload				*/
	NULL,						/* destroy				*/
	NULL,						/* ui_info				*/
	NULL,						/* extra_info			*/
	&prefs_info,				/* prefs_info			*/
	NULL,						/* actions				*/	
	NULL,						/* padding				*/
	NULL,						/* padding				*/
	NULL,						/* padding				*/
	NULL						/* padding				*/
};

static void init_plugin( PurplePlugin *plugin )
{
	purple_prefs_add_none( PLUGIN_PREFS );
	purple_prefs_add_int( PLUGIN_PREF_ON, 1 );
}

PURPLE_INIT_PLUGIN( PLUGIN_STATIC_NAME, init_plugin, info )
