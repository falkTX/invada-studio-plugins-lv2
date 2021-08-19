/* 

    This widget provides phase meters

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

#include "stdlib.h"
#include "string.h"
#include "widgets.h"
#include "meter-phase.h"


static void 	inv_phase_meter_class_init(InvPhaseMeterClass *klass);
static void 	inv_phase_meter_init(InvPhaseMeter *meter);
static void 	inv_phase_meter_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_phase_meter_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_phase_meter_realize(GtkWidget *widget);
static gboolean inv_phase_meter_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_phase_meter_paint(GtkWidget *widget, gint mode);
static void	inv_phase_meter_destroy(GtkObject *object);
static void	inv_phase_meter_colour(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led);


GtkType
inv_phase_meter_get_type(void)
{
	static GType inv_phase_meter_type = 0;
	char *name;
	int i;


	if (!inv_phase_meter_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvPhaseMeterClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_phase_meter_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvPhaseMeter),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_phase_meter_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvPhaseMeter-%p-%d",inv_phase_meter_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_phase_meter_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_phase_meter_type;
}

void
inv_phase_meter_set_bypass(InvPhaseMeter *meter, gint num)
{
	if(meter->bypass != num) {
		meter->bypass = num;
		meter->phase=0;
	}
}

void
inv_phase_meter_set_phase(InvPhaseMeter *meter, float num)
{
	meter->phase = num;
	if(GTK_WIDGET_REALIZED(meter))
		inv_phase_meter_paint(GTK_WIDGET(meter),INV_PHASE_METER_DRAW_DATA);
}

GtkWidget * inv_phase_meter_new()
{
	return GTK_WIDGET(gtk_type_new(inv_phase_meter_get_type()));
}


static void
inv_phase_meter_class_init(InvPhaseMeterClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_phase_meter_realize;
	widget_class->size_request = inv_phase_meter_size_request;
	widget_class->size_allocate = inv_phase_meter_size_allocate;
	widget_class->expose_event = inv_phase_meter_expose;

	object_class->destroy = inv_phase_meter_destroy;
}


static void
inv_phase_meter_init(InvPhaseMeter *meter)
{
	meter->bypass = INV_PLUGIN_ACTIVE;
	meter->phase = 0;

	meter->mOff0.R =0.1;	meter->mOff0.G =0.1;	meter->mOff0.B =0.4;
	meter->mOn0.R  =-0.1;	meter->mOn0.G  =-0.1;	meter->mOn0.B  =0.6;

	meter->mOff30.R =0.2; 	meter->mOff30.G =0.3;	meter->mOff30.B =0.4;
	meter->mOn30.R  =-0.1;	meter->mOn30.G  =0.3;	meter->mOn30.B  =0.6;

	meter->mOff45.R =0.2; 	meter->mOff45.G =0.4;	meter->mOff45.B =0.2;
	meter->mOn45.R  =0.1;	meter->mOn45.G  =0.6;	meter->mOn45.B  =-0.1;

	meter->mOff60.R  =0.5;	meter->mOff60.G  =0.5;	meter->mOff60.B  =0.0;
	meter->mOn60.R   =0.5;	meter->mOn60.G   =0.5;	meter->mOn60.B   =0.0;

	meter->mOff90.R=0.4;	meter->mOff90.G=0.2;	meter->mOff90.B=0.0;
	meter->mOn90.R =0.6;	meter->mOn90.G =0.0;	meter->mOn90.B =0.0;

	meter->font_size=0;

	gtk_widget_set_tooltip_markup(GTK_WIDGET(meter),"<span size=\"8000\">Phase Meter.</span>");
}


static void
inv_phase_meter_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_PHASE_METER(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 388;
	requisition->height = 36;
}


static void
inv_phase_meter_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_PHASE_METER(widget));
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
inv_phase_meter_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_PHASE_METER(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = 388;
	attributes.height = 36;

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
inv_phase_meter_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_PHASE_METER(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_phase_meter_paint(widget,INV_PHASE_METER_DRAW_ALL);

	return FALSE;
}


static void
inv_phase_meter_paint(GtkWidget *widget, gint mode)
{
	gint 			phase;
	gint 			bypass;

	gint 			i;
	cairo_t 		*cr;
	float 			Pon;
	GtkStyle		*style;

	char 			label[10];
	struct colour		led;
	cairo_text_extents_t 	extents;

	style   = gtk_widget_get_style(widget);
	bypass = INV_PHASE_METER(widget)->bypass;
	phase  = (gint)((INV_PHASE_METER(widget)->phase*57.295779506)+0.2);

	cr = gdk_cairo_create(widget->window);

	if(INV_PHASE_METER(widget)->font_size==0) {
		INV_PHASE_METER(widget)->font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}

	if(mode==INV_PHASE_METER_DRAW_ALL) {
		cairo_set_source_rgb(cr, 0, 0, 0);
		cairo_paint(cr);

		cairo_new_path(cr);

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);

		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 0, 35);
		cairo_line_to(cr, 0, 0);
		cairo_line_to(cr, 387, 0);
		cairo_stroke(cr);

		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 0, 35);
		cairo_line_to(cr, 387, 35);
		cairo_line_to(cr, 387, 0);
		cairo_stroke(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
		cairo_new_path(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		}
		
		cairo_rectangle(cr, 14, 21, 1, 2);
		cairo_fill(cr);

		cairo_rectangle(cr, 104, 21, 1, 2);
		cairo_fill(cr);

		cairo_rectangle(cr, 194, 21, 1, 2);
		cairo_fill(cr);

		cairo_rectangle(cr, 284, 21, 1, 2);
		cairo_fill(cr);

		cairo_rectangle(cr, 374, 21, 1, 2);
		cairo_fill(cr);


		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
		} else {
			cairo_set_source_rgb(cr, 1, 1, 1);
		}

		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_PHASE_METER(widget)->font_size);

		strcpy(label,"-90");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,13-(extents.width/2),31);
		cairo_show_text(cr,label);

		strcpy(label,"-45");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,103-(extents.width/2),31);
		cairo_show_text(cr,label);

		strcpy(label,"0");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,194-(extents.width/2),31);
	
		cairo_show_text(cr,label);

		strcpy(label,"45");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,284-(extents.width/2),31);
		cairo_show_text(cr,label);

		strcpy(label,"90");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,374-(extents.width/2),31);
		cairo_show_text(cr,label);


	}


	inv_phase_meter_colour(widget, bypass, 0, 1, &led);
	cairo_set_source_rgb(cr, led.R, led.G, led.B);
	cairo_rectangle(cr, 194, 5, 1, 14);

	for ( i = 1; i <= 90; i++) 
	{
		Pon = i <= phase ? 1 : 0;

		inv_phase_meter_colour(widget, bypass, i, Pon, &led);
		cairo_set_source_rgb(cr, led.R, led.G, led.B);
		cairo_rectangle(cr, 194+(i*2), 5, 1, 14);
		cairo_fill(cr);
		cairo_rectangle(cr, 194-(i*2), 5, 1, 14);
		cairo_fill(cr);
	}
  	cairo_destroy(cr);
}


static void
inv_phase_meter_destroy(GtkObject *object)
{
	InvPhaseMeter *meter;
	InvPhaseMeterClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_PHASE_METER(object));

	meter = INV_PHASE_METER(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static void	
inv_phase_meter_colour(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led)
{
	float r1,r2;

	struct colour  mOff0  = INV_PHASE_METER(widget)->mOff0;
	struct colour  mOn0   = INV_PHASE_METER(widget)->mOn0;
	struct colour  mOff30 = INV_PHASE_METER(widget)->mOff30;
	struct colour  mOn30  = INV_PHASE_METER(widget)->mOn30;
	struct colour  mOff45 = INV_PHASE_METER(widget)->mOff45;
	struct colour  mOn45  = INV_PHASE_METER(widget)->mOn45;
	struct colour  mOff60 = INV_PHASE_METER(widget)->mOff60;
	struct colour  mOn60  = INV_PHASE_METER(widget)->mOn60;
	struct colour  mOff90 = INV_PHASE_METER(widget)->mOff90;
	struct colour  mOn90  = INV_PHASE_METER(widget)->mOn90;


	if(pos < 30) 
	{
		r1=(30.0-(float)pos)/30.0;
		r2=(float)pos/30.0;
		led->R=(r1 * mOff0.R + (r2 * mOff30.R))  + (on * ((r1 * mOn0.R) + (r2 * mOn30.R))) ;
		led->G=(r1 * mOff0.G + (r2 * mOff30.G))  + (on * ((r1 * mOn0.G) + (r2 * mOn30.G))) ;
		led->B=(r1 * mOff0.B + (r2 * mOff30.B))  + (on * ((r1 * mOn0.B) + (r2 * mOn30.B))) ;
	} 

	else if (pos < 45)
	{
		r1=(45.0-(float)pos)/15.0;
		r2=((float)pos-30.0)/15.0;
		led->R=(r1 * mOff30.R + (r2 * mOff45.R))  + (on * ((r1 * mOn30.R) + (r2 * mOn45.R))) ;
		led->G=(r1 * mOff30.G + (r2 * mOff45.G))  + (on * ((r1 * mOn30.G) + (r2 * mOn45.G))) ;
		led->B=(r1 * mOff30.B + (r2 * mOff45.B))  + (on * ((r1 * mOn30.B) + (r2 * mOn45.B))) ;
	}

	else if (pos < 60)
	{
		r1=(60.0-(float)pos)/15.0;
		r2=((float)pos-45.0)/15.0;
		led->R=(r1 * mOff45.R + (r2 * mOff60.R))  + (on * ((r1 * mOn45.R) + (r2 * mOn60.R))) ;
		led->G=(r1 * mOff45.G + (r2 * mOff60.G))  + (on * ((r1 * mOn45.G) + (r2 * mOn60.G))) ;
		led->B=(r1 * mOff45.B + (r2 * mOff60.B))  + (on * ((r1 * mOn45.B) + (r2 * mOn60.B))) ;
	}
	else
	{
		r1=(90.0-(float)pos)/30.0;
		r2=((float)pos-60.0)/30.0;
		led->R=(r1 * mOff60.R + (r2 * mOff90.R))  + (on * ((r1 * mOn60.R) + (r2 * mOn90.R))) ;
		led->G=(r1 * mOff60.G + (r2 * mOff90.G))  + (on * ((r1 * mOn60.G) + (r2 * mOn90.G))) ;
		led->B=(r1 * mOff60.B + (r2 * mOff90.B))  + (on * ((r1 * mOn60.B) + (r2 * mOn90.B))) ;
	}	

	if(bypass==INV_PLUGIN_BYPASS) {
		led->R=(led->R+led->G+led->B)/3;
		led->G=led->R;
		led->B=led->R;
	}
}

