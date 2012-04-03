/*
 *	operator-name-cbs-home-item (operator name item)
 *	Copyright (C) 2011 Nicolai Hess/Jonathan Wilson
 *	
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *	
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *	GNU General Public License for more details.
 *	
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <libconnui.h>
#include <gconf/gconf-client.h>
#include "operator-name-cbs-home-item.h"
#include "smsutil.h"

#define PHONE_NET_SERVICE "com.nokia.phone.net"
#define PHONE_NET_PATH "/com/nokia/phone/net"
#define PHONE_NET_IFACE "Phone.Net"
#define PHONE_SIM_SERVICE "com.nokia.phone.SIM"
#define PHONE_SIM_PATH "/com/nokia/phone/SIM"
#define PHONE_SIM_IFACE "Phone.Sim"

#define GET_OPERATOR_NAME "get_operator_name"
#define GET_REGISTRATION_STATUS "get_registration_status"
#define GET_SERVICE_PROVIDER_NAME "get_service_provider_name"
#define GET_SERVICE_PROVIDER_INFO "get_service_provider_info"

#define OPERATOR_NAME_CBS_PATH "/apps/connui-cellular"
#define OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED OPERATOR_NAME_CBS_PATH "/cbsms_display_enabled"
#define OPERATOR_NAME_CBS_LOGGING_ENABLED OPERATOR_NAME_CBS_PATH "/logging_enabled"
#define OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED OPERATOR_NAME_CBS_PATH "/name_logging_enabled"

#define OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, TYPE_OPERATOR_NAME_CBS_HOME_ITEM, OperatorNameCBSHomeItemPrivate))
gboolean cbslog = FALSE;
gboolean namelog = FALSE;
gboolean cbsms = FALSE;
static void get_operator_name(OperatorNameCBSHomeItemPrivate *priv,struct network_state *state);

struct _OperatorNameCBSHomeItemPrivate
{
	GtkWidget* label;
	DBusGConnection* dbus_conn;
	gchar* cell_name;
	gchar* display_name;
	gchar* operator_name;
	gchar* service_provider_name;
	gchar reg_status;
	gchar rat_name;
	gchar *operator_code;
	int status;
	gboolean flightmode;
	guint32 service_provider_name_type;
	guint32 cell_id;
	gboolean name;
	guint gconfnotify_id;
	GConfClient *gconf_client;
};

HD_DEFINE_PLUGIN_MODULE(OperatorNameCBSHomeItem, operator_name_cbs_home_item, HD_TYPE_HOME_PLUGIN_ITEM);

static char timestamp[50];
static char *get_timestamp()
{
	time_t ltime;
	struct tm *Tm;
	ltime=time(NULL);
	Tm=localtime(&ltime);
	sprintf(timestamp,"%02d:%02d:%02d ",Tm->tm_hour,Tm->tm_min,Tm->tm_sec);
	return timestamp;
}

static DBusHandlerResult
_dbus_message_filter_func(DBusConnection* connection,
				DBusMessage* message,
				void* user_data)
{
	OperatorNameCBSHomeItem* home_item = OPERATOR_NAME_CBS_HOME_ITEM(user_data);
	if(home_item)
	{
		OperatorNameCBSHomeItemPrivate* priv = OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(home_item);
		if(priv && !priv->flightmode)
		{
			if(dbus_message_is_signal(message,"Phone.SMS","IncomingCBS"))
			{
				char *data;
				int count;
				int serial;
				int message_id;
				int coding;
				int page;
				int info_length;
				gboolean ret;
				struct cbs cbs;
				GSList *l;
				char lang[3];
				char *utf8;
				dbus_message_get_args(message,NULL,DBUS_TYPE_ARRAY,DBUS_TYPE_BYTE,&data,
									&count,DBUS_TYPE_UINT32,&serial,DBUS_TYPE_UINT32,
									&message_id,DBUS_TYPE_BYTE,&coding,DBUS_TYPE_BYTE,
									&page,DBUS_TYPE_BYTE,&info_length,DBUS_TYPE_INVALID);
				unsigned char pdu[88];
				pdu[0] = (unsigned char)((serial >> 8) & 0xFF);
				pdu[1] = (unsigned char)(serial & 0xFF);
				pdu[2] = (unsigned char)((message_id >> 8) & 0xFF);
				pdu[3] = (unsigned char)(message_id & 0xFF);
				pdu[4] = (unsigned char)coding;
				pdu[5] = (unsigned char)page;
				memset(&pdu[6],0,82);
				memcpy(&pdu[6],data,count);
				ret = cbs_decode(pdu,88,&cbs);
				l = g_slist_append(NULL, &cbs);
				utf8 = cbs_decode_text(l, lang);
				if (cbs.message_identifier == 50 && strncmp(utf8, "@@@@@", 5) != 0)
				{
					g_free(priv->cell_name);
					priv->cell_name = g_strdup(utf8);
					priv->name = FALSE;
					char *name = g_strdup_printf("%s %s",priv->display_name,priv->cell_name);
					gtk_label_set_text(GTK_LABEL(priv->label), name);
					gtk_widget_queue_draw(priv->label);
					DBusGProxy *dbus_g_proxy = dbus_g_proxy_new_for_name(priv->dbus_conn, PHONE_NET_SERVICE, PHONE_NET_PATH, PHONE_NET_IFACE);
					if(dbus_g_proxy)
					{
						gint32 net_err = -1;
						guchar status;
						guint16 lac;
						guint16 network_type;
						guint16 services;
						guint32 cellid;
						guint32 operator_code;
						guint32 country_code;
						if(dbus_g_proxy_call(dbus_g_proxy, GET_REGISTRATION_STATUS, NULL,
							G_TYPE_INVALID,
							G_TYPE_UCHAR, &status,
							G_TYPE_UINT, &lac,
							G_TYPE_UINT, &cellid,
							G_TYPE_UINT, &operator_code,
							G_TYPE_UINT, &country_code,
							G_TYPE_UCHAR, &network_type,
							G_TYPE_UCHAR, &services,
							G_TYPE_INT, &net_err,
							G_TYPE_INVALID) && net_err == 0)
						{
							if (priv->cell_id != cellid)
							{
								if (cbslog)
								{
									FILE *f = fopen("/home/user/cbsms.log","at");
									fprintf(f,"%scell id change 2 %d\n",get_timestamp(),cellid);
									fclose(f);
								}
								priv->cell_id = cellid;
							}
						}
					}
					if (cbslog)
					{
						FILE *f = fopen("/home/user/cbsms.log","at");
						fprintf(f,"%sincoming cell tower name %s, cell id %d\n",get_timestamp(),name,priv->cell_id);
						fclose(f);
					}
				}
				g_free(utf8);
				g_slist_free(l);
			}
		}
	}
	return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static void widget_flightmode_cb(gboolean offline, gpointer user_data)
{
	OperatorNameCBSHomeItem* home_item = OPERATOR_NAME_CBS_HOME_ITEM(user_data);
	if(home_item)
	{
		OperatorNameCBSHomeItemPrivate* priv = OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(home_item);
		if(priv)
		{
			if (offline)
			{
				if (cbslog)
				{
					FILE *f = fopen("/home/user/cbsms.log","at");
					fprintf(f,"%sflightmode clear cell name\n",get_timestamp());
					fclose(f);
				}
				g_free(priv->cell_name);
				priv->cell_name = g_strdup("");
				g_free(priv->display_name);
				priv->display_name = g_strdup("");
				priv->reg_status = -1;
				g_free(priv->operator_code);
				priv->operator_code = g_strdup("");
				priv->status = -1;
				priv->rat_name = -1;
				priv->cell_id = -1;
				priv->name = FALSE;
				gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
				gtk_widget_queue_draw(priv->label);
			}
			priv->flightmode = offline;
		}
	}
}

static gboolean name_handler(gpointer user_data)
{
	OperatorNameCBSHomeItem* home_item = OPERATOR_NAME_CBS_HOME_ITEM(user_data);
	if(home_item)
	{
		OperatorNameCBSHomeItemPrivate* priv = OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(home_item);
		if(priv && priv->name)
		{
			g_free(priv->cell_name);
			priv->cell_name = g_strdup("");
		}
	}
	return FALSE;
}

static void widget_net_status_cb(struct network_state *state, gpointer user_data)
{
	OperatorNameCBSHomeItem* home_item = OPERATOR_NAME_CBS_HOME_ITEM(user_data);
	if(home_item)
	{
		OperatorNameCBSHomeItemPrivate* priv = OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(home_item);
		if(priv && !priv->flightmode)
		{
			if (state->network_reg_status != priv->reg_status)
			{
				priv->reg_status = state->network_reg_status;
				if (cbslog)
				{
					FILE *f = fopen("/home/user/cbsms.log","at");
					fprintf(f,"%snetwork_reg_status clear cell name\n",get_timestamp());
					fclose(f);
				}
				g_free(priv->cell_name);
				priv->cell_name = g_strdup("");
				g_free(priv->display_name);
				priv->display_name = g_strdup("");
				gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
				gtk_widget_queue_draw(priv->label);
				g_free(priv->operator_code);
				priv->operator_code = g_strdup("");
				priv->status = -1;
				priv->rat_name = -1;
				priv->cell_id = -1;
				priv->name = FALSE;
			}
			if (state->network)
			{
				if (state->network->network_service_status != priv->status && priv->reg_status < 3)
				{
					if (cbslog)
					{
						FILE *f = fopen("/home/user/cbsms.log","at");
						fprintf(f,"%snetwork_service_status clear cell name\n",get_timestamp());
						fclose(f);
					}
					priv->status = state->network->network_service_status;
					g_free(priv->cell_name);
					priv->cell_name = g_strdup("");
					gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
					gtk_widget_queue_draw(priv->label);
					priv->cell_id = -1;
					priv->rat_name = -1;
					priv->name = FALSE;
				}
				if (state->network->operator_code && strcmp(state->network->operator_code,priv->operator_code) && priv->reg_status < 3 && priv->status < 2)
				{
					if (cbslog)
					{
						FILE *f = fopen("/home/user/cbsms.log","at");
						fprintf(f,"%soperator_code clear cell name\n",get_timestamp());
						fclose(f);
					}
					g_free(priv->operator_code);
					priv->operator_code = g_strdup(state->network->operator_code);
					get_operator_name(priv,state);
					g_free(priv->cell_name);
					priv->cell_name = g_strdup("");
					gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
					gtk_widget_queue_draw(priv->label);
					priv->cell_id = -1;
					priv->rat_name = -1;
					priv->name = FALSE;
				}
			}
			if (state->rat_name != priv->rat_name && priv->reg_status < 3 && priv->status < 2)
			{
				if (cbslog)
				{
					FILE *f = fopen("/home/user/cbsms.log","at");
					fprintf(f,"%srat_name clear cell name\n",get_timestamp());
					fclose(f);
				}
				priv->rat_name = state->rat_name;
				g_free(priv->cell_name);
				priv->cell_name = g_strdup("");
				gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
				gtk_widget_queue_draw(priv->label);
				priv->cell_id = -1;
				priv->name = FALSE;
			}
			if (priv->cell_id != state->cell_id && priv->reg_status < 3 && priv->status < 2)
			{
				if (cbslog)
				{
					FILE *f = fopen("/home/user/cbsms.log","at");
					fprintf(f,"%scell id change %d\n",get_timestamp(),state->cell_id);
					fclose(f);
				}
				priv->cell_id = state->cell_id;
				priv->name = TRUE;
				g_timeout_add_seconds(5,name_handler,(gpointer)home_item);
			}
		}
	}
}

static void
_disable_dbus_filter_func(DBusConnection* conn, OperatorNameCBSHomeItem* plugin)
{
	if(conn)
	{
		dbus_connection_remove_filter(conn, _dbus_message_filter_func, NULL);
	}
}

static void
_set_dbus_filter_func(DBusConnection* conn, OperatorNameCBSHomeItem* plugin)
{
	if(conn)
	{
		dbus_bus_add_match(conn,"type='signal',interface='Phone.SMS'", NULL);
		dbus_connection_add_filter(conn, _dbus_message_filter_func, plugin, NULL);
	}
}

static void gconf_changed_func(GConfClient *gconf_client, guint cnxn_id, GConfEntry *entry, OperatorNameCBSHomeItem* home_item)
{
	if (gconf_entry_get_value (entry) != NULL && gconf_entry_get_value (entry)->type == GCONF_VALUE_BOOL)
	{
		gboolean new_setting = gconf_value_get_bool(gconf_entry_get_value(entry));
		if (!strcmp(gconf_entry_get_key(entry),OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED))
		{
			if (cbsms != new_setting)
			{
				cbsms = new_setting;
				if (cbslog)
				{
					FILE *f = fopen("/home/user/cbsms.log","at");
					fprintf(f,"%scell broadcast setting changed\n",get_timestamp());
					fclose(f);
				}
				if (new_setting)
				{
					_set_dbus_filter_func(dbus_g_connection_get_connection(home_item->priv->dbus_conn), home_item);
				}
				else
				{
					g_free(home_item->priv->cell_name);
					home_item->priv->cell_name = g_strdup("");
					gtk_label_set_text(GTK_LABEL(home_item->priv->label), home_item->priv->display_name);
					gtk_widget_queue_draw(home_item->priv->label);
					_disable_dbus_filter_func(dbus_g_connection_get_connection(home_item->priv->dbus_conn), home_item);
				}
			}
		}
		if (!strcmp(gconf_entry_get_key(entry),OPERATOR_NAME_CBS_LOGGING_ENABLED))
		{
			cbslog = new_setting;
			FILE *f = fopen("/home/user/cbsms.log","at");
			fprintf(f,"%scbsms logging changed\n",get_timestamp());
			fclose(f);
		}
		if (!strcmp(gconf_entry_get_key(entry),OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED))
		{
			namelog = new_setting;
			FILE *f = fopen("/home/user/cbsms.log","at");
			fprintf(f,"%sname loging changed\n",get_timestamp());
			fclose(f);
		}
	}
}

static void get_operator_name(OperatorNameCBSHomeItemPrivate *priv,struct network_state* state)
{
	if (namelog)
	{
		FILE *f = fopen("/home/user/opername.log","at");
		fprintf(f,"%sStart Operator Name\n",get_timestamp());
		fclose(f);
	}
	if (state->operator_name && state->operator_name[0])
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sstate->operator_name %s\n",get_timestamp(),state->operator_name);
			fclose(f);
		}
		if (!priv->operator_name || strcmp(priv->operator_name,state->operator_name))
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%spriv->operator_name %s\n",get_timestamp(),priv->operator_name);
				fclose(f);
			}
			if (priv->operator_name)
			{
				g_free(priv->operator_name);
			}
			priv->operator_name = g_strdup(state->operator_name);
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%spriv->operator_name changed to %s\n",get_timestamp(),priv->operator_name);
				fclose(f);
			}
		}
	}
	gint32 error;
	gchar *operator = connui_cell_net_get_operator_name(state->network,TRUE,&error);
	if (namelog)
	{
		FILE *f = fopen("/home/user/opername.log","at");
		fprintf(f,"%soperator %s error %d\n",get_timestamp(),operator,error);
		fclose(f);
	}
	guchar code[3];
	code[0] = (state->network->country_code[0] - 0x30) | ((state->network->country_code[1] - 0x30) << 4);
	if (error)
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%serror is set, clearing operator\n",get_timestamp());
			fclose(f);
		}
		operator = 0;
	}
	gchar *oper = priv->operator_name;
	if (namelog)
	{
		FILE *f = fopen("/home/user/opername.log","at");
		fprintf(f,"%soper is %s\n",get_timestamp(),oper);
		fclose(f);
	}
	if (!oper)
	{
		oper = operator;
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%soper is changed to %s\n",get_timestamp(),oper);
			fclose(f);
		}
	}
	code[1] = (strlen(state->network->operator_code) == 2) ? 0xF0 : ((state->network->operator_code[2] << 4) & 0xF0) | (state->network->country_code[2] - 0x30);
	code[2] = (state->network->operator_code[0] - 0x30) | ((state->network->operator_code[1] - 0x30) << 4);
	if (namelog)
	{
		FILE *f = fopen("/home/user/opername.log","at");
		fprintf(f,"%scode %d %d %d\n",get_timestamp(),code[0],code[1],code[2]);
		fclose(f);
	}
	if (!priv->service_provider_name)
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sservice provider name get\n",get_timestamp());
			fclose(f);
		}
		priv->service_provider_name = connui_cell_sim_get_service_provider(&priv->service_provider_name_type,&error);
		if (priv->service_provider_name == 0)
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name null\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup(oper);
			goto set_label;
		}
	}
	if (priv->service_provider_name[0] == 0)
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sservice provider name empty\n",get_timestamp());
			fclose(f);
		}
		oper = g_strdup(oper);
set_label:
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sset operator name %s\n",get_timestamp(),oper);
				fclose(f);
			}
			g_free(priv->display_name);
			priv->display_name = g_strdup(oper);
			gtk_label_set_text(GTK_LABEL(priv->label), priv->display_name);
			gtk_widget_queue_draw(priv->label);
			g_free(oper);
			g_free(operator);
			return;
		}
	}
	else if ((priv->reg_status == 0) || (connui_cell_sim_is_network_in_service_provider_info(&error,code)))
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sservice provider name network match %s type %d\n",get_timestamp(),priv->service_provider_name,priv->service_provider_name_type);
			fclose(f);
		}
		if (!priv->service_provider_name_type & 1)
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name type 1\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup(priv->service_provider_name);
			goto set_label;
		}
		if (!strcasecmp(oper,priv->service_provider_name))
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name match\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup(priv->service_provider_name);
			goto set_label;
		}
		else
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name fail match\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup_printf("%s%s%s",oper," ",priv->service_provider_name);
			goto set_label;
		}
	}
	else
	{
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sservice provider name network match fail %s type %d\n",get_timestamp(),priv->service_provider_name,priv->service_provider_name_type);
			fclose(f);
		}
		if (!priv->service_provider_name_type & 2)
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name type 2\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup(oper);
			goto set_label;
		}
		if (!strcasecmp(oper,priv->service_provider_name))
		{
			if (namelog)
			{
				FILE *f = fopen("/home/user/opername.log","at");
				fprintf(f,"%sservice provider name match network fail\n",get_timestamp());
				fclose(f);
			}
			oper = g_strdup(oper);
			goto set_label;
		}
		if (namelog)
		{
			FILE *f = fopen("/home/user/opername.log","at");
			fprintf(f,"%sservice provider name fail match network fail\n",get_timestamp());
			fclose(f);
		}
		oper = g_strdup_printf("%s%s%s",oper," ",priv->service_provider_name);
		goto set_label;
	}
}

static gboolean operator_name_cbs_home_item_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
	cairo_t *cr;
	cr = gdk_cairo_create (GDK_DRAWABLE (widget->window));
	gdk_cairo_region (cr, event->region);
	cairo_clip (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
	cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.0);
	cairo_paint (cr);
	cairo_destroy (cr);
	return GTK_WIDGET_CLASS(operator_name_cbs_home_item_parent_class)->expose_event (widget, event);
}

static void operator_name_cbs_home_item_realize(GtkWidget *widget)
{
	GdkScreen *screen = gtk_widget_get_screen (widget);
	gtk_widget_set_colormap (widget, gdk_screen_get_rgba_colormap (screen));
	gtk_widget_set_app_paintable (widget, TRUE);
	GTK_WIDGET_CLASS(operator_name_cbs_home_item_parent_class)->realize (widget);
}

static void
operator_name_cbs_home_item_init(OperatorNameCBSHomeItem* home_item)
{
	home_item->priv = OPERATOR_NAME_CBS_HOME_ITEM_GET_PRIVATE(home_item);
	home_item->priv->cell_name = g_strdup("");
	home_item->priv->reg_status = -1;
	home_item->priv->operator_code = g_strdup("");
	home_item->priv->status = -1;
	home_item->priv->rat_name = -1;
	home_item->priv->cell_id = -1;
	home_item->priv->dbus_conn = hd_home_plugin_item_get_dbus_g_connection(&home_item->hitem, DBUS_BUS_SYSTEM, NULL);
	home_item->priv->label = gtk_label_new(NULL);
	home_item->priv->flightmode = FALSE;
	gtk_widget_set_name(home_item->priv->label, "HomeCellularLabel");
	hildon_helper_set_logical_font(home_item->priv->label, "SystemFont");
	gtk_label_set_text(GTK_LABEL(home_item->priv->label), "");
	gtk_misc_set_alignment(GTK_MISC(home_item->priv->label), 0.0f, 1.0f);
	gtk_misc_set_padding(GTK_MISC(home_item->priv->label), 0, 12);
	gtk_widget_show(GTK_WIDGET(home_item->priv->label));
	gtk_container_add(GTK_CONTAINER(home_item), GTK_WIDGET(home_item->priv->label));
	if (home_item->priv->gconf_client == NULL)
	{
		home_item->priv->gconf_client = gconf_client_get_default();
	}
	if (home_item->priv->gconfnotify_id == 0)
	{
		gconf_client_add_dir(home_item->priv->gconf_client, OPERATOR_NAME_CBS_PATH, GCONF_CLIENT_PRELOAD_NONE, NULL);
		home_item->priv->gconfnotify_id = gconf_client_notify_add(home_item->priv->gconf_client, OPERATOR_NAME_CBS_PATH, (GConfClientNotifyFunc) gconf_changed_func, home_item, NULL, NULL);
	}
	cbslog = gconf_client_get_bool(home_item->priv->gconf_client, OPERATOR_NAME_CBS_LOGGING_ENABLED, NULL);
	namelog = gconf_client_get_bool(home_item->priv->gconf_client, OPERATOR_NAME_CBS_NAME_LOGGING_ENABLED, NULL);
	if (namelog)
	{
		FILE *f = fopen("/home/user/opername.log","at");
		fprintf(f,"%sname logging enabled\n",get_timestamp());
		fclose(f);
	}
	cbsms = gconf_client_get_bool(home_item->priv->gconf_client, OPERATOR_NAME_CBS_CBSMS_DISPLAY_ENABLED, NULL);
	if (cbsms)
	{
		_set_dbus_filter_func(dbus_g_connection_get_connection(home_item->priv->dbus_conn), home_item);
	}
	connui_cell_net_status_register(widget_net_status_cb,home_item);
	connui_flightmode_status(widget_flightmode_cb,home_item);
	if (cbslog)
	{
		FILE *f = fopen("/home/user/cbsms.log","at");
		fprintf(f,"%scbsms logging enabled\n",get_timestamp());
		fprintf(f,"%sinit clear cell name\n",get_timestamp());
		fclose(f);
	}
}

static void
operator_name_cbs_home_item_finalize(GObject* object)
{
	OperatorNameCBSHomeItem* home_item = OPERATOR_NAME_CBS_HOME_ITEM(object);
	connui_cell_net_status_close(widget_net_status_cb);
	connui_flightmode_close(widget_flightmode_cb);
	if (home_item->priv->gconfnotify_id != 0)
	{
		gconf_client_notify_remove(home_item->priv->gconf_client, home_item->priv->gconfnotify_id);
		gconf_client_remove_dir(home_item->priv->gconf_client, OPERATOR_NAME_CBS_PATH, NULL);
		home_item->priv->gconfnotify_id = 0;
	}
	if (home_item->priv->gconf_client != NULL)
	{
		gconf_client_clear_cache(home_item->priv->gconf_client);
		g_object_unref (G_OBJECT(home_item->priv->gconf_client));
		home_item->priv->gconf_client = NULL;
	}
	if (cbsms)
	{
		_disable_dbus_filter_func(dbus_g_connection_get_connection(home_item->priv->dbus_conn), home_item);
	}
	G_OBJECT_CLASS(operator_name_cbs_home_item_parent_class)->finalize(object);
}

static void
operator_name_cbs_home_item_class_finalize(OperatorNameCBSHomeItemClass* klass)
{
}

static void
operator_name_cbs_home_item_class_init(OperatorNameCBSHomeItemClass* klass)
{
	g_type_class_add_private(klass, sizeof(OperatorNameCBSHomeItemPrivate));
	GTK_WIDGET_CLASS(klass)->realize = operator_name_cbs_home_item_realize;
	GTK_WIDGET_CLASS(klass)->expose_event = operator_name_cbs_home_item_expose_event;
	G_OBJECT_CLASS(klass)->finalize = (GObjectFinalizeFunc)operator_name_cbs_home_item_finalize;
	gtk_rc_parse_string("style \"HomeCellularLabel\" = \"osso-color-themeing\" { fg[NORMAL] = \"#FFFFFF\" engine \"sapwood\" { shadowcolor = \"#000000\" }} widget \"*.HomeCellularLabel\" style \"HomeCellularLabel\"");
}
