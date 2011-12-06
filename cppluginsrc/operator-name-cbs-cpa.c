/*
 *  operator-name-cbs-cpa (control panel plugin)
 *  Copyright (C) 2011 Nicolai Hess/Jonathan Wilson
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>

#include <gtk/gtk.h>
#include <glib.h>
#include <libintl.h>
#include <locale.h>
#include <gconf/gconf-client.h>

#define OPERATOR_NAME_CBS_PATH "/apps/connui-cellular"
#define OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED OPERATOR_NAME_CBS_PATH "/cbsms_display_enabled"
#define OPERATOR_NAME_CBS_LOGGING_ENABLED OPERATOR_NAME_CBS_PATH "/logging_enabled"
#define OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED OPERATOR_NAME_CBS_PATH "/name_logging_enabled"

osso_return_t execute(osso_context_t *osso,
		      gpointer data,
		      gboolean user_activated)
{
	GtkWidget* dialog = gtk_dialog_new_with_buttons("Operator Name Widget",GTK_WINDOW(data),GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR,dgettext("hildon-libs", "wdgt_bd_save"), GTK_RESPONSE_ACCEPT,NULL);
	GConfClient* gconf_client = gconf_client_get_default();
	g_assert(GCONF_IS_CLIENT(gconf_client));
	GtkWidget* box = gtk_vbox_new(TRUE, 0);
	GtkWidget* log_enabled = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
	GtkWidget* name_log_enabled = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
	GtkWidget* cbsms_enabled = hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT);
	gtk_button_set_label(GTK_BUTTON(log_enabled),"Logging Enabled");
	gtk_button_set_label(GTK_BUTTON(name_log_enabled),"Name Logging Enabled");
	gtk_button_set_label(GTK_BUTTON(cbsms_enabled),"Cell Broadcast Enabled");
	hildon_check_button_set_active(HILDON_CHECK_BUTTON(log_enabled),gconf_client_get_bool(gconf_client, OPERATOR_NAME_CBS_LOGGING_ENABLED, NULL));
	hildon_check_button_set_active(HILDON_CHECK_BUTTON(name_log_enabled),gconf_client_get_bool(gconf_client, OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED, NULL));
	hildon_check_button_set_active(HILDON_CHECK_BUTTON(cbsms_enabled),gconf_client_get_bool(gconf_client, OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED, NULL));
	gtk_box_pack_start(GTK_BOX(box), log_enabled, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), name_log_enabled, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), cbsms_enabled, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), box, TRUE, TRUE, 0);
	gtk_widget_show_all(dialog);
	guint response = gtk_dialog_run(GTK_DIALOG(dialog));
	if(response == GTK_RESPONSE_ACCEPT)
	{
		gconf_client_set_bool(gconf_client,OPERATOR_NAME_CBS_LOGGING_ENABLED,hildon_check_button_get_active(HILDON_CHECK_BUTTON(log_enabled)),NULL);
		gconf_client_set_bool(gconf_client,OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED,hildon_check_button_get_active(HILDON_CHECK_BUTTON(name_log_enabled)),NULL);
		gconf_client_set_bool(gconf_client,OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED,hildon_check_button_get_active(HILDON_CHECK_BUTTON(cbsms_enabled)),NULL);
	}
	gtk_widget_destroy(dialog);
	g_object_unref(gconf_client);
	return OSSO_OK;
}

osso_return_t save_state(osso_context_t *osso, gpointer data)
{
	return OSSO_OK;
}
