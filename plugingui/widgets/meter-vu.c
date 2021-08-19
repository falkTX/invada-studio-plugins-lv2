/* 

    This widget provides vu meters

    (c) Fraser Stuart 2009

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "widgets.h"
#include "meter-vu.h"


static void 	inv_vu_meter_class_init(InvVuMeterClass *klass);
static void 	inv_vu_meter_init(InvVuMeter *meter);
static void 	inv_vu_meter_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_vu_meter_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_vu_meter_realize(GtkWidget *widget);
static gboolean inv_vu_meter_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_vu_meter_paint(GtkWidget *widget, gint mode);
static void	inv_vu_meter_destroy(GtkObject *object);



GtkType
inv_vu_meter_get_type(void)
{
	static GType inv_vu_meter_type = 0;
	char *name;
	int i;


	if (!inv_vu_meter_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvVuMeterClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_vu_meter_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvVuMeter),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_vu_meter_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvVuMeter-%p-%d",inv_vu_meter_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_vu_meter_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_vu_meter_type;
}

void
inv_vu_meter_set_bypass(InvVuMeter *meter, gint num)
{
	if(meter->bypass != num) {
		meter->bypass = num;
		meter->value=0;
	}
}

void
inv_vu_meter_set_value(InvVuMeter *meter, float num)
{
	meter->value = num;
	if(GTK_WIDGET_REALIZED(meter) && meter->value != meter->lastvalue)
		inv_vu_meter_paint(GTK_WIDGET(meter),INV_VU_METER_DRAW_DATA);
}

GtkWidget * inv_vu_meter_new()
{
	return GTK_WIDGET(gtk_type_new(inv_vu_meter_get_type()));
}

void
inv_vu_meter_set_headroom(InvVuMeter *meter, gint num)
{
	if(meter->headroom != num) {
		meter->headroom = num;
		meter->scale=pow(10,((float)meter->headroom+4.0)/20.0);
	}
}

static void
inv_vu_meter_class_init(InvVuMeterClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_vu_meter_realize;
	widget_class->size_request = inv_vu_meter_size_request;
	widget_class->size_allocate = inv_vu_meter_size_allocate;
	widget_class->expose_event = inv_vu_meter_expose;

	object_class->destroy = inv_vu_meter_destroy;
}


static void
inv_vu_meter_init(InvVuMeter *meter)
{
	meter->bypass = INV_PLUGIN_ACTIVE;
	meter->value = 0;

	gtk_widget_set_tooltip_markup(GTK_WIDGET(meter),"<span size=\"8000\">VU Meter.</span>");


	meter->headroom=9;
	meter->scale=pow(10,((float)meter->headroom+4)/20);

	meter->cx=91.0;
	meter->cy=130.0;

	meter->r[0]=101.0;  //needle
	meter->r[1]=104.0;  //markings lower
	meter->r[2]=112.0;  //markings upper
	meter->r[3]=114.0;  //labels


	meter->a[0]=(-INV_PI/2)-0.7943;  //-inf db
	meter->a[1]=(-INV_PI/2)+(1-0.7943); //0db
	meter->a[2]=(-INV_PI/2)+0.7943; //max scale (approx +4db)
	meter->a[3]=(-INV_PI/2)-0.83;  //- needle clip
	meter->a[4]=(-INV_PI/2)+0.83;  //+ needle clip

	meter->dbm20[0].x=meter->cx+meter->r[1]*sin(0.1-0.7943); 
	meter->dbm20[0].y=meter->cy-meter->r[1]*cos(0.1-0.7943);	
	meter->dbm20[1].x=meter->cx+meter->r[2]*sin(0.1-0.7943);
	meter->dbm20[1].y=meter->cy-meter->r[2]*cos(0.1-0.7943);
	meter->dbm20[2].x=meter->cx+meter->r[3]*sin(0.1-0.7943);
	meter->dbm20[2].y=meter->cy-meter->r[3]*cos(0.1-0.7943);

	meter->dbm10[0].x=meter->cx+meter->r[1]*sin(0.3162277-0.7943);	
	meter->dbm10[0].y=meter->cy-meter->r[1]*cos(0.3162277-0.7943);	
	meter->dbm10[1].x=meter->cx+meter->r[2]*sin(0.3162277-0.7943);
	meter->dbm10[1].y=meter->cy-meter->r[2]*cos(0.3162277-0.7943);
	meter->dbm10[2].x=meter->cx+meter->r[3]*sin(0.3162277-0.7943);
	meter->dbm10[2].y=meter->cy-meter->r[3]*cos(0.3162277-0.7943);

	meter->dbm07[0].x=meter->cx+meter->r[1]*sin(0.4466835-0.7943); 	
	meter->dbm07[0].y=meter->cy-meter->r[1]*cos(0.4466835-0.7943);	
	meter->dbm07[1].x=meter->cx+meter->r[2]*sin(0.4466835-0.7943);
	meter->dbm07[1].y=meter->cy-meter->r[2]*cos(0.4466835-0.7943);
	meter->dbm07[2].x=meter->cx+meter->r[3]*sin(0.4466835-0.7943);
	meter->dbm07[2].y=meter->cy-meter->r[3]*cos(0.4466835-0.7943);

	meter->dbm05[0].x=meter->cx+meter->r[1]*sin(0.5623413-0.7943); 	
	meter->dbm05[0].y=meter->cy-meter->r[1]*cos(0.5623413-0.7943);	
	meter->dbm05[1].x=meter->cx+meter->r[2]*sin(0.5623413-0.7943);
	meter->dbm05[1].y=meter->cy-meter->r[2]*cos(0.5623413-0.7943);
	meter->dbm05[2].x=meter->cx+meter->r[3]*sin(0.5623413-0.7943);
	meter->dbm05[2].y=meter->cy-meter->r[3]*cos(0.5623413-0.7943);

	meter->dbm03[0].x=meter->cx+meter->r[1]*sin(0.7079457-0.7943); 
	meter->dbm03[0].y=meter->cy-meter->r[1]*cos(0.7079457-0.7943);		
	meter->dbm03[1].x=meter->cx+meter->r[2]*sin(0.7079457-0.7943);
	meter->dbm03[1].y=meter->cy-meter->r[2]*cos(0.7079457-0.7943);
	meter->dbm03[2].x=meter->cx+meter->r[3]*sin(0.7079457-0.7943);
	meter->dbm03[2].y=meter->cy-meter->r[3]*cos(0.7079457-0.7943);

	meter->dbm02[0].x=meter->cx+meter->r[1]*sin(0.7943282-0.7943); 	
	meter->dbm02[0].y=meter->cy-meter->r[1]*cos(0.7943282-0.7943);	
	meter->dbm02[1].x=meter->cx+meter->r[2]*sin(0.7943282-0.7943);
	meter->dbm02[1].y=meter->cy-meter->r[2]*cos(0.7943282-0.7943);
	meter->dbm02[2].x=meter->cx+meter->r[3]*sin(0.7943282-0.7943);
	meter->dbm02[2].y=meter->cy-meter->r[3]*cos(0.7943282-0.7943);

	meter->dbm01[0].x=meter->cx+meter->r[1]*sin(0.8912509-0.7943); 	
	meter->dbm01[0].y=meter->cy-meter->r[1]*cos(0.8912509-0.7943);	
	meter->dbm01[1].x=meter->cx+meter->r[2]*sin(0.8912509-0.7943);
	meter->dbm01[1].y=meter->cy-meter->r[2]*cos(0.8912509-0.7943);
	meter->dbm01[2].x=meter->cx+meter->r[3]*sin(0.8912509-0.7943);
	meter->dbm01[2].y=meter->cy-meter->r[3]*cos(0.8912509-0.7943);

	meter->db00[0].x=meter->cx+meter->r[1]*sin(1.0-0.7943); 
	meter->db00[0].y=meter->cy-meter->r[1]*cos(1.0-0.7943);		
	meter->db00[1].x=meter->cx+meter->r[2]*sin(1.0-0.7943);	
	meter->db00[1].y=meter->cy-meter->r[2]*cos(1.0-0.7943);
	meter->db00[2].x=meter->cx+meter->r[3]*sin(1.0-0.7943);	
	meter->db00[2].y=meter->cy-meter->r[3]*cos(1.0-0.7943);

	meter->dbp01[0].x=meter->cx+meter->r[1]*sin(1.1220184-0.7943); 	
	meter->dbp01[0].y=meter->cy-meter->r[1]*cos(1.1220184-0.7943);	
	meter->dbp01[1].x=meter->cx+meter->r[2]*sin(1.1220184-0.7943);
	meter->dbp01[1].y=meter->cy-meter->r[2]*cos(1.1220184-0.7943);
	meter->dbp01[2].x=meter->cx+meter->r[3]*sin(1.1220184-0.7943);
	meter->dbp01[2].y=meter->cy-meter->r[3]*cos(1.1220184-0.7943);

	meter->dbp02[0].x=meter->cx+meter->r[1]*sin(1.2589254-0.7943); 	
	meter->dbp02[0].y=meter->cy-meter->r[1]*cos(1.2589254-0.7943);	
	meter->dbp02[1].x=meter->cx+meter->r[2]*sin(1.2589254-0.7943);
	meter->dbp02[1].y=meter->cy-meter->r[2]*cos(1.2589254-0.7943);
	meter->dbp02[2].x=meter->cx+meter->r[3]*sin(1.2589254-0.7943);
	meter->dbp02[2].y=meter->cy-meter->r[3]*cos(1.2589254-0.7943);

	meter->dbp03[0].x=meter->cx+meter->r[1]*sin(1.4125375-0.7943);
	meter->dbp03[0].y=meter->cy-meter->r[1]*cos(1.4125375-0.7943);
 	meter->dbp03[1].x=meter->cx+meter->r[2]*sin(1.4125375-0.7943);
	meter->dbp03[1].y=meter->cy-meter->r[2]*cos(1.4125375-0.7943);
 	meter->dbp03[2].x=meter->cx+meter->r[3]*sin(1.4125375-0.7943);
	meter->dbp03[2].y=meter->cy-meter->r[3]*cos(1.4125375-0.7943);

	meter->cp[0].x=60;
	meter->cp[0].y=104;
	meter->cp[1].x=166;
	meter->cp[1].y=104;

	meter->label_font_size=0;
	meter->scale_font_size=0;

}


static void
inv_vu_meter_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_VU_METER(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 183;
	requisition->height = 105;
}


static void
inv_vu_meter_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_VU_METER(widget));
	g_return_if_fail(allocation != NULL);

	widget->allocation = *allocation;

	if (GTK_WIDGET_REALIZED(widget)) {
		gdk_window_move_resize(
		   widget->window,
		   allocation->x, allocation->y,
		   allocation->width, allocation->height
		);
	}
}


static void
inv_vu_meter_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_VU_METER(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = 183;
	attributes.height = 105;

	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y;

	widget->window = gdk_window_new(
	  gtk_widget_get_parent_window (widget),
	  & attributes, attributes_mask
	);

	gdk_window_set_user_data(widget->window, widget);

	widget->style = gtk_style_attach(widget->style, widget->window);
	gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
}


static gboolean
inv_vu_meter_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_VU_METER(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_vu_meter_paint(widget,INV_VU_METER_DRAW_ALL);

	return FALSE;
}


static void
inv_vu_meter_paint(GtkWidget *widget, gint mode)
{
	float 			rawvalue,value;
	gint 			bypass;

	cairo_t 		*cr;
	GtkStyle		*style;

	char 			label[10];
	cairo_text_extents_t 	extents;


	bypass   = INV_VU_METER(widget)->bypass;
	rawvalue = INV_VU_METER(widget)->value;
	value    = rawvalue*INV_VU_METER(widget)->scale;

	style   = gtk_widget_get_style(widget);

	cr = gdk_cairo_create(widget->window);


	if(INV_VU_METER(widget)->label_font_size==0) {
		INV_VU_METER(widget)->label_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,9.1,"0");
	}

	if(INV_VU_METER(widget)->scale_font_size==0) {
		INV_VU_METER(widget)->scale_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,7.1,"0");
	}

	if(mode==INV_VU_METER_DRAW_ALL) {

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
		} else {
			cairo_set_source_rgb(cr, 1.0, 0.90, 0.65);
		}
		cairo_paint(cr);

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);

		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 0, 104);
		cairo_line_to(cr, 0, 0);
		cairo_line_to(cr, 182, 0);
		cairo_stroke(cr);

		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 0, 104);
		cairo_line_to(cr, 182, 104);
		cairo_line_to(cr, 182, 0);
		cairo_stroke(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);

		cairo_set_line_width(cr,1.5);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.12, 0.12, 0.12);
		} else {
			cairo_set_source_rgb(cr, 0.15, 0.12, 0.08);
		}
		
		//VU label
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(cr,INV_VU_METER(widget)->label_font_size);
		strcpy(label,"VU");
//		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,6,99);
		cairo_show_text(cr,label);

		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_VU_METER(widget)->scale_font_size);


		//scale marks
		cairo_move_to(cr,INV_VU_METER(widget)->dbm20[0].x,INV_VU_METER(widget)->dbm20[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm20[1].x,INV_VU_METER(widget)->dbm20[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm10[0].x,INV_VU_METER(widget)->dbm10[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm10[1].x,INV_VU_METER(widget)->dbm10[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm07[0].x,INV_VU_METER(widget)->dbm07[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm07[1].x,INV_VU_METER(widget)->dbm07[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm05[0].x,INV_VU_METER(widget)->dbm05[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm05[1].x,INV_VU_METER(widget)->dbm05[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm03[0].x,INV_VU_METER(widget)->dbm03[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm03[1].x,INV_VU_METER(widget)->dbm03[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm02[0].x,INV_VU_METER(widget)->dbm02[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm02[1].x,INV_VU_METER(widget)->dbm02[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbm01[0].x,INV_VU_METER(widget)->dbm01[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbm01[1].x,INV_VU_METER(widget)->dbm01[1].y);

		cairo_stroke(cr);

		//arc
		cairo_set_line_width(cr,2);
		cairo_arc(cr,INV_VU_METER(widget)->cx,INV_VU_METER(widget)->cy,INV_VU_METER(widget)->r[1],INV_VU_METER(widget)->a[0],INV_VU_METER(widget)->a[1]);
		cairo_stroke(cr);


		//+labels
		strcpy(label,"20");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm20[2].x-extents.width,INV_VU_METER(widget)->dbm20[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"10");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm10[2].x-extents.width,INV_VU_METER(widget)->dbm10[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"7");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm07[2].x-extents.width,INV_VU_METER(widget)->dbm07[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"5");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm05[2].x-extents.width,INV_VU_METER(widget)->dbm05[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"3");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm03[2].x-extents.width,INV_VU_METER(widget)->dbm03[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"2");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,INV_VU_METER(widget)->dbm02[2].x-(extents.width/2),INV_VU_METER(widget)->dbm02[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"1");
		cairo_move_to(cr,INV_VU_METER(widget)->dbm01[2].x,INV_VU_METER(widget)->dbm01[2].y);
		cairo_show_text(cr,label);


		// minus sign
		cairo_rectangle(cr,6,11,12,2);
		cairo_fill(cr);


		//scale marks
		cairo_set_line_width(cr,1.5);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.39, 0.39, 0.39);
		} else {
			cairo_set_source_rgb(cr, 0.80, 0.22, 0.15);
		}
		

		cairo_move_to(cr,INV_VU_METER(widget)->db00[0].x,INV_VU_METER(widget)->db00[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->db00[1].x,INV_VU_METER(widget)->db00[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbp01[0].x,INV_VU_METER(widget)->dbp01[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbp01[1].x,INV_VU_METER(widget)->dbp01[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbp02[0].x,INV_VU_METER(widget)->dbp02[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbp02[1].x,INV_VU_METER(widget)->dbp02[1].y);

		cairo_move_to(cr,INV_VU_METER(widget)->dbp03[0].x,INV_VU_METER(widget)->dbp03[0].y);
		cairo_line_to(cr,INV_VU_METER(widget)->dbp03[1].x,INV_VU_METER(widget)->dbp03[1].y);

		cairo_stroke(cr);

		//arc
		cairo_set_line_width(cr,4);
		cairo_arc(cr,INV_VU_METER(widget)->cx,INV_VU_METER(widget)->cy,INV_VU_METER(widget)->r[1]+1,INV_VU_METER(widget)->a[1],INV_VU_METER(widget)->a[2]);
		cairo_stroke(cr);


		//+labels
		strcpy(label,"0");
		cairo_move_to(cr,INV_VU_METER(widget)->db00[2].x,INV_VU_METER(widget)->db00[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"1");
		cairo_move_to(cr,INV_VU_METER(widget)->dbp01[2].x,INV_VU_METER(widget)->dbp01[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"2");
		cairo_move_to(cr,INV_VU_METER(widget)->dbp02[2].x,INV_VU_METER(widget)->dbp02[2].y);
		cairo_show_text(cr,label);

		strcpy(label,"3");
		cairo_move_to(cr,INV_VU_METER(widget)->dbp03[2].x,INV_VU_METER(widget)->dbp03[2].y);
		cairo_show_text(cr,label);

		//plus sign
		cairo_rectangle(cr,166,11,12,2);
		cairo_fill(cr);
		cairo_rectangle(cr,171,6,2,12);
		cairo_fill(cr);

	}

	if(bypass==INV_PLUGIN_BYPASS) {
		cairo_set_source_rgb(cr, 0.85, 0.85, 0.85);
	} else {
		cairo_set_source_rgb(cr, 1.0, 0.90, 0.65);
	}
	cairo_move_to(cr,INV_VU_METER(widget)->cp[0].x,INV_VU_METER(widget)->cp[0].y);
	cairo_arc(cr,INV_VU_METER(widget)->cx,INV_VU_METER(widget)->cy,INV_VU_METER(widget)->r[0]+1,INV_VU_METER(widget)->a[3],INV_VU_METER(widget)->a[4]);
	cairo_line_to(cr,INV_VU_METER(widget)->cp[1].x,INV_VU_METER(widget)->cp[1].y);
	cairo_line_to(cr,INV_VU_METER(widget)->cp[0].x,INV_VU_METER(widget)->cp[0].y);
	cairo_fill_preserve(cr);
	cairo_clip(cr);

	cairo_set_line_width(cr,1.5);
	cairo_set_source_rgb(cr, 0, 0, 0);
	


	if(value < 1.5886) {
		cairo_move_to(cr,INV_VU_METER(widget)->cx,INV_VU_METER(widget)->cy);
		cairo_line_to(cr,INV_VU_METER(widget)->cx+(INV_VU_METER(widget)->r[0]*sin(value-0.7943)),
				 INV_VU_METER(widget)->cy-(INV_VU_METER(widget)->r[0]*cos(value-0.7943)) );
	} else {
		cairo_curve_to(cr,INV_VU_METER(widget)->cx,
				  INV_VU_METER(widget)->cy,
				  INV_VU_METER(widget)->cx+(INV_VU_METER(widget)->r[0]*2*sin(value-0.7943)/3),
				  INV_VU_METER(widget)->cy-(INV_VU_METER(widget)->r[0]*2*cos(value-0.7943)/3),
				  INV_VU_METER(widget)->cx+(INV_VU_METER(widget)->r[0]*0.7133),
				  INV_VU_METER(widget)->cy-(INV_VU_METER(widget)->r[0]*0.7008) );
	}
  	cairo_stroke(cr);
	
  	cairo_destroy(cr);

	INV_VU_METER(widget)->lastvalue=rawvalue;
}


static void
inv_vu_meter_destroy(GtkObject *object)
{
	InvVuMeter *meter;
	InvVuMeterClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_VU_METER(object));

	meter = INV_VU_METER(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}


