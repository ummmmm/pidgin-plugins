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

#define PLUGIN_ID			"core-fliptext-messages"
#define PLUGIN_NAME			"Flip Text"
#define PLUGIN_STATIC_NAME	"fliptext"
#define PLUGIN_AUTHOR		"David Carver <dcarver@miva.com>"
#define PLUGIN_SUMMARY		"Flips lowercase text upside down in outgoing messages"
#define PLUGIN_DESCRIPTION	"Flips lowercase text upside down in outgoing messages"
#define PLUGIN_PREFS		"/plugins/core/" PLUGIN_STATIC_NAME
#define PLUGIN_PREF_ON		PLUGIN_PREFS "/on"
#define PLUGIN_PREF_BLIST	PLUGIN_STATIC_NAME

typedef enum
{
	BLIST_DEFAULT,
	BLIST_ALWAYS,
	BLIST_NEVER
} FlipTextBlistSettings;

static void flip_char( const char *reverse, char **buffer, int *buffer_length, int *buffer_pos )
{
	long int unicode;
	char temp[ 5 ];
	char *endp;

	temp[ 0 ] = reverse[ 0 ];
	temp[ 1 ] = reverse[ 1 ];
	temp[ 2 ] = reverse[ 2 ];
	temp[ 3 ] = reverse[ 3 ];
	temp[ 4 ] = '\0';

	unicode = strtol( temp, &endp, 16 );

	if ( ( size_t ) ( endp - temp ) != ( 4 * sizeof( char ) ) )
	{
		return;
	}

	if ( unicode <= 0x7F )
	{
		( *buffer )[ ( *buffer_pos )++ ]	= unicode;
		*buffer_length						+= 1;
	}
	else if ( unicode <= 0x7FF )
	{
		( *buffer )[ ( *buffer_pos )++ ]	= ( 0xC0 | ( 0x1F & ( unicode >> 6 ) ) );
		( *buffer )[ ( *buffer_pos )++ ]	= ( 0x80 | ( 0x3F & unicode ) );
		*buffer_length						+= 2;
	}
	else if ( unicode < 0xD800 || unicode > 0xDFFF )
	{
		( *buffer )[ ( *buffer_pos )++ ] 	= ( 0xE0 | ( 0xF & ( unicode >> 12 ) ) );
		( *buffer )[ ( *buffer_pos )++ ] 	= ( 0x80 | ( 0x3F & ( unicode >> 6 ) ) );
		( *buffer )[ ( *buffer_pos )++ ]	= ( 0x80 | ( 0x3F & unicode ) );
		*buffer_length						+= 3;
	}
}

static void append_char( const char *data, int data_length, char **buffer, int *buffer_length, int *buffer_size, int *buffer_pos )
{
	char *temp;

	if ( ( *buffer_length + 10 ) > *buffer_size )
	{
		*buffer_size 	= *buffer_length + 100;
		temp 			= malloc( *buffer_size );

		memcpy( temp, *buffer, *buffer_length );

		if ( buffer )
		{
			free( *buffer );
		}

		*buffer 		= temp;
	}

	switch ( data_length )
	{
		case 4 	:
		{
			flip_char( data, buffer, buffer_length, buffer_pos );

			break;
		}
		case 1 	:
		{
			( *buffer )[ ( *buffer_pos )++ ] = data[ 0 ];
			*buffer_length += 1;

			break;
		}
		default	: break;
	}
}

static void flip_text( char **original_buffer )
{
	char c;
	char *buffer, *temp;
	int i, buffer_length, buffer_size, buffer_pos, original_buffer_length;

	original_buffer_length	= strlen( *original_buffer );
	buffer_length 			= 0;
	buffer_pos 				= 0;
	buffer_size				= 0;
	buffer					= 0;
	temp 					= *original_buffer;

	for( i = original_buffer_length - 1; i >= 0; i-- )
	{
		c = temp[ i ];

		switch ( c )
		{
			case 'a' 	: append_char( "0250", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'c' 	: append_char( "0254", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'e' 	: append_char( "01DD", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'f' 	: append_char( "025F", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'h' 	: append_char( "0265", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'i' 	: append_char( "0131", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'j' 	: append_char( "0638", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'k' 	: append_char( "029E", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'l' 	: append_char( "05DF", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'm' 	: append_char( "026F", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'r' 	: append_char( "0279", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 't' 	: append_char( "0287", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'v' 	: append_char( "028C", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'w' 	: append_char( "028D", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'y' 	: append_char( "028E", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '?' 	: append_char( "00BF", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '!' 	: append_char( "00A1", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '.' 	: append_char( "02D9", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '_' 	: append_char( "203E", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case ';' 	: append_char( "061B", 	4, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;

			case 'b' 	: append_char( "q", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'd' 	: append_char( "p", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'g' 	: append_char( "b", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'n' 	: append_char( "u", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'o' 	: append_char( "o", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'p' 	: append_char( "d", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'q' 	: append_char( "b", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 's' 	: append_char( "s", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'u' 	: append_char( "n", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'x' 	: append_char( "x", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case 'z' 	: append_char( "z", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '[' 	: append_char( "]", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case ']' 	: append_char( "[", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '(' 	: append_char( ")", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case ')' 	: append_char( "(", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '{' 	: append_char( "}", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '}' 	: append_char( "{", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '\''	: append_char( ",", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case ',' 	: append_char( "\"", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '6' 	: append_char( "9", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			case '9' 	: append_char( "6", 	1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
			default		: append_char( &c, 		1, &buffer, &buffer_length, &buffer_size, &buffer_pos ); break;
		}
	}

	buffer[ buffer_length ] = '\0';
	*original_buffer		= buffer;

	free( temp );
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
		purple_debug_info( PLUGIN_STATIC_NAME, "Sent as flipped text\n" );
		flip_text( buffer );
	}
}

static void save_flip_text_settings_cb( PurpleBlistNode *node, int choice )
{
	if ( PURPLE_BLIST_NODE_IS_BUDDY( node) )
	{
		node = purple_blist_node_get_parent( node );
	}

	g_return_if_fail( PURPLE_BLIST_NODE_IS_CONTACT( node ) );

	purple_blist_node_set_int( node, PLUGIN_PREF_BLIST, choice );
}

static void set_flip_text_settings_cb( PurpleBlistNode *node, gpointer plugin )
{
	char *message;

	if ( PURPLE_BLIST_NODE_IS_BUDDY( node ) )
	{
		node = purple_blist_node_get_parent(node);
	}

	g_return_if_fail( PURPLE_BLIST_NODE_IS_CONTACT( node ) );

	message = g_strdup_printf( "When sending an instant message to %s", purple_contact_get_alias( ( PurpleContact * ) node ) );

	purple_request_choice( plugin,													/* Handle */
						   "Set Flip Text Setting",									/* Title */
						   message,													/* Primary */
						   NULL,													/* Secondary */
						   purple_blist_node_get_int( node, PLUGIN_PREF_BLIST ),	/* Default value */
						   "Save",													/* OK text */
						   G_CALLBACK( save_flip_text_settings_cb ),				/* OK callback */
						   "Cancel",												/* Cancel text */
						   NULL,													/* Cancel callback */
						   NULL,													/* Account */
						   NULL,													/* Who */
						   NULL,													/* Conversation */
						   node,													/* User data to pass to the callback */
						   "Use Default Settings",									/* First radio */
						   BLIST_DEFAULT,											/* Radio value */
						   "Always use flipped text",								/* Second radio */
						   BLIST_ALWAYS,											/* Radio value */
						   "Never use flipped text",								/* Third radio */
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

	action 	= purple_menu_action_new( "Use Flip Text...", PURPLE_CALLBACK( set_flip_text_settings_cb ), plugin, NULL );
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
	pref 	= purple_plugin_pref_new_with_name_and_label( PLUGIN_PREF_ON, "Send instant messages using flipped text when:\n" );

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
