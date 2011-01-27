/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2. Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id$
 */

#include <libtransmission/transmission.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "conf.h" /* gtr_pref_string_get */
#include "hig.h"
#include "relocate.h"
#include "util.h"

#define DATA_KEY "gtr-relocate-data"

static char * previousLocation = NULL;

struct relocate_dialog_data
{
    int done;
    tr_bool do_move;
    TrCore * core;
    GSList * torrent_ids;
    GtkWidget * message_dialog;
    GtkWidget * chooser_dialog;
};

static void
data_free( gpointer gdata )
{
    struct relocate_dialog_data * data = gdata;
    g_slist_free( data->torrent_ids );
    g_free( data );
}

/***
****
***/

static void
startMovingNextTorrent( struct relocate_dialog_data * data )
{
    char * str;
    const int id = GPOINTER_TO_INT( data->torrent_ids->data );

    tr_session * session = tr_core_session( data->core );

    tr_torrent * tor = tr_torrentFindFromId( session, id );
    if( tor != NULL )
        tr_torrentSetLocation( tor, previousLocation, data->do_move, NULL, &data->done );

    data->torrent_ids = g_slist_delete_link( data->torrent_ids,
                                             data->torrent_ids );

    str = g_strdup_printf( _( "Moving \"%s\"" ), tr_torrentName( tor ) );
    gtk_message_dialog_set_markup( GTK_MESSAGE_DIALOG( data->message_dialog ), str );
    g_free( str );
}

/* every once in awhile, check to see if the move is done.
 * if so, delete the dialog */
static gboolean
onTimer( gpointer gdata )
{
    struct relocate_dialog_data * data = gdata;
    const int done = data->done;

    if( done == TR_LOC_ERROR )
    {
        const int flags = GTK_DIALOG_MODAL
                        | GTK_DIALOG_DESTROY_WITH_PARENT;
        GtkWidget * w = gtk_message_dialog_new( GTK_WINDOW( data->message_dialog ),
                                                flags,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_CLOSE,
                                                "%s",
                                                _( "Couldn't move torrent" ) );
        gtk_dialog_run( GTK_DIALOG( w ) );
        gtk_widget_destroy( GTK_WIDGET( data->message_dialog ) );
        return FALSE;
    }
    else if( done == TR_LOC_DONE )
    {
        if( data->torrent_ids != NULL )
        {
            startMovingNextTorrent( data );
        }
        else
        {
            gtk_widget_destroy( GTK_WIDGET( data->chooser_dialog ) );
            return FALSE;
        }
    }

    return TRUE; /* keep looping */
}

static void
onResponse( GtkDialog * dialog, int response, gpointer unused UNUSED )
{
    if( response == GTK_RESPONSE_APPLY )
    {
        GtkWidget * w;
        GObject * d = G_OBJECT( dialog );
        struct relocate_dialog_data * data = g_object_get_data( d, DATA_KEY );
        GtkFileChooser * chooser = g_object_get_data( d, "chooser" );
        GtkToggleButton * move_tb = g_object_get_data( d, "move_rb" );
        char * location = gtk_file_chooser_get_filename( chooser );

        data->do_move = gtk_toggle_button_get_active( move_tb );

        /* pop up a dialog saying that the work is in progress */
        w = gtk_message_dialog_new( GTK_WINDOW( dialog ),
                                    GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_INFO,
                                    GTK_BUTTONS_CLOSE,
                                    NULL );
        gtk_message_dialog_format_secondary_text( GTK_MESSAGE_DIALOG( w ), _( "This may take a moment..." ) );
        gtk_dialog_set_response_sensitive( GTK_DIALOG( w ), GTK_RESPONSE_CLOSE, FALSE );
        gtk_widget_show( w );

        /* remember this location so that it can be the default next time */
        g_free( previousLocation );
        previousLocation = location;

        /* start the move and periodically check its status */
        data->message_dialog = w;
        data->done = TR_LOC_DONE;
        onTimer( data );
        gtr_timeout_add_seconds( 1, onTimer, data );
    }
    else
    {
        gtk_widget_destroy( GTK_WIDGET( dialog ) );
    }
}

GtkWidget*
gtr_relocate_dialog_new( GtkWindow * parent,
                         TrCore    * core,
                         GSList    * torrent_ids )
{
    int row;
    GtkWidget * w;
    GtkWidget * d;
    GtkWidget * t;
    struct relocate_dialog_data * data;

    d = gtk_dialog_new_with_buttons( _( "Set Torrent Location" ), parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT |
                                     GTK_DIALOG_MODAL,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                                     NULL );
    gtk_dialog_set_default_response( GTK_DIALOG( d ),
                                     GTK_RESPONSE_CANCEL );
    gtk_dialog_set_alternative_button_order( GTK_DIALOG( d ),
                                             GTK_RESPONSE_APPLY,
                                             GTK_RESPONSE_CANCEL,
                                             -1 );
    g_signal_connect( d, "response", G_CALLBACK( onResponse ), NULL );

    row = 0;
    t = hig_workarea_create( );
    hig_workarea_add_section_title( t, &row, _( "Location" ) );

    if( previousLocation == NULL )
        previousLocation = g_strdup( gtr_pref_string_get( TR_PREFS_KEY_DOWNLOAD_DIR ) );
    w = gtk_file_chooser_button_new( _( "Set Torrent Location" ), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
    gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( w ), previousLocation );
    g_object_set_data( G_OBJECT( d ), "chooser", w );
    hig_workarea_add_row( t, &row, _( "Torrent _location:" ), w, NULL );
    w = gtk_radio_button_new_with_mnemonic( NULL, _( "_Move from the current folder" ) );
    g_object_set_data( G_OBJECT( d ), "move_rb", w );
    hig_workarea_add_wide_control( t, &row, w );
    w = gtk_radio_button_new_with_mnemonic_from_widget( GTK_RADIO_BUTTON( w ), _( "Local data is _already there" ) );
    hig_workarea_add_wide_control( t, &row, w );
    hig_workarea_finish( t, &row );
    gtr_dialog_set_content( GTK_DIALOG( d ), t );

    data = g_new0( struct relocate_dialog_data, 1 );
    data->core = core;
    data->torrent_ids = torrent_ids;
    data->chooser_dialog = d;
    g_object_set_data_full( G_OBJECT( d ), DATA_KEY, data, data_free );

    return d;
}

static void
onRenameEntryActivate( GtkEntry * e UNUSED, gpointer vdialog )
{
    GtkDialog * d = GTK_DIALOG( vdialog );
    gtk_dialog_response( d, GTK_RESPONSE_APPLY );
}

GtkWidget *
gtr_rename_top_dialog_new( GtkWindow  * parent,
                           TrCore     * core UNUSED,
                           tr_torrent * tor )
{
    const tr_info * info = tr_torrentInfo( tor );
    GtkWidget * d, * e, * t, * l;
    int row;

    d = gtk_dialog_new_with_buttons( _( "Rename Torrent Directory" ), parent,
                                     GTK_DIALOG_DESTROY_WITH_PARENT |
                                     GTK_DIALOG_MODAL,
                                     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                     GTK_STOCK_APPLY, GTK_RESPONSE_APPLY,
                                     NULL );
    gtk_dialog_set_default_response( GTK_DIALOG( d ),
                                     GTK_RESPONSE_CANCEL );
    gtk_dialog_set_alternative_button_order( GTK_DIALOG( d ),
                                             GTK_RESPONSE_APPLY,
                                             GTK_RESPONSE_CANCEL,
                                             -1 );
    row = 0;
    t = hig_workarea_create( );
    hig_workarea_add_section_title( t, &row, _( "Rename Directory" ) );

    l = g_object_new( GTK_TYPE_LABEL,
                      "selectable", TRUE,
                      "ellipsize", PANGO_ELLIPSIZE_END,
                      NULL );
    gtk_label_set_text( GTK_LABEL( l ), info->name );
    hig_workarea_add_row( t, &row, _( "Original name:" ), l, NULL );

    e = gtk_entry_new( );
    gtk_entry_set_width_chars( GTK_ENTRY( e ), 64 );
    gtk_entry_set_text( GTK_ENTRY( e ), tr_torrentName( tor ) );
    g_object_set_data( G_OBJECT( d ), "rename-entry", e );
    g_signal_connect( e, "activate",
                      G_CALLBACK( onRenameEntryActivate ), d );
    hig_workarea_add_wide_control( t, &row, e );

    hig_workarea_finish( t, &row );
    gtr_dialog_set_content( GTK_DIALOG( d ), t );

    return d;
}

const gchar *
gtr_rename_top_dialog_get_new_name( GtkWidget * dialog )
{
    GtkWidget * entry;
    entry = g_object_get_data( G_OBJECT( dialog ), "rename-entry" );
    return entry ? gtk_entry_get_text( GTK_ENTRY( entry ) ) : NULL;
}
