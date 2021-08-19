/* 

    This widget provides lamps

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
#include "lamp.h"

static void 	inv_lamp_class_init(InvLampClass *klass);
static void 	inv_lamp_init(InvLamp *lamp);
static void 	inv_lamp_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_lamp_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_lamp_realize(GtkWidget *widget);
static gboolean inv_lamp_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_lamp_paint(GtkWidget *widget, gint mode);
static void	inv_lamp_destroy(GtkObject *object);
static void	inv_lamp_colour(GtkWidget *widget, float value, struct colour *rc, struct colour *cc);


GtkType
inv_lamp_get_type(void)
{
	static GType inv_lamp_type = 0;
	char *name;
	int i;


	if (!inv_lamp_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvLampClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_lamp_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvLamp),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_lamp_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvLamp-%p-%d",inv_lamp_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_lamp_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_lamp_type;
}

void
inv_lamp_set_scale(InvLamp *lamp, float num)
{
	lamp->scale = num;
}

void
inv_lamp_set_value(InvLamp *lamp, float num)
{
	lamp->value = num;
	if(lamp->value != lamp->lastValue) {
		if(GTK_WIDGET_REALIZED(lamp))
			inv_lamp_paint(GTK_WIDGET(lamp),INV_LAMP_DRAW_DATA);
	}
}

void inv_lamp_set_tooltip(InvLamp *lamp, gchar *tip)
{
	gtk_widget_set_tooltip_markup(GTK_WIDGET(lamp),tip);
}


GtkWidget * inv_lamp_new()
{
	return GTK_WIDGET(gtk_type_new(inv_lamp_get_type()));
}


static void
inv_lamp_class_init(InvLampClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_lamp_realize;
	widget_class->size_request = inv_lamp_size_request;
	widget_class->size_allocate = inv_lamp_size_allocate;
	widget_class->expose_event = inv_lamp_expose;

	object_class->destroy = inv_lamp_destroy;
}


static void
inv_lamp_init(InvLamp *lamp)
{
	lamp->scale = 1;
	lamp->value = 0;
	lamp->lastValue=0;

	lamp->l0_r.R =0.1;	lamp->l0_r.G =0.0;	lamp->l0_r.B =0.0;
	lamp->l0_c.R =0.2;	lamp->l0_c.G =0.0;	lamp->l0_c.B =0.0;

	lamp->l1_r.R =0.2;	lamp->l1_r.G =0.0;	lamp->l1_r.B =0.0;
	lamp->l1_c.R =1.0;	lamp->l1_c.G =0.0;	lamp->l1_c.B =0.0;

	lamp->l2_r.R =0.3;	lamp->l2_r.G =0.0;	lamp->l2_r.B =0.0;
	lamp->l2_c.R =1.0;	lamp->l2_c.G =0.5;	lamp->l2_c.B =0.0;

	lamp->l3_r.R =0.4;	lamp->l3_r.G =0.0;	lamp->l3_r.B =0.0;
	lamp->l3_c.R =1.0;	lamp->l3_c.G =1.0;	lamp->l3_c.B =0.0;

	lamp->l4_r.R =0.5;	lamp->l4_r.G =0.0;	lamp->l4_r.B =0.0;
	lamp->l4_c.R =1.0;	lamp->l4_c.G =1.0;	lamp->l4_c.B =0.5;   
}


static void
inv_lamp_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_LAMP(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 32;
	requisition->height = 32;
}


static void
inv_lamp_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_LAMP(widget));
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
inv_lamp_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_LAMP(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = 32;
	attributes.height = 32;

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
inv_lamp_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_LAMP(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_lamp_paint(widget,INV_LAMP_DRAW_ALL);

	return FALSE;
}


static void
inv_lamp_paint(GtkWidget *widget, gint mode)
{
	cairo_t 	*cr;
	float 		xc,yc,r;
	struct colour	rc,cc;
	GtkStyle	*style;
	cairo_pattern_t *pat;

	style = gtk_widget_get_style(widget);

	float scale = INV_LAMP(widget)->scale;
	float value = INV_LAMP(widget)->value;

	cr = gdk_cairo_create(widget->window);

	xc=16.0;
	yc=16.0;
	r=9.5;

	switch(mode) {
		case INV_LAMP_DRAW_ALL:

			cairo_arc(cr,xc,yc,13,0,2*INV_PI);
			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_fill_preserve(cr);

			pat = cairo_pattern_create_linear (0.0, 0.0,  32.0, 32.0);
			cairo_pattern_add_color_stop_rgba (pat, 0.0, 1.00, 1.00, 1.00, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.2, 0.91, 0.89, 0.83, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.5, 0.43, 0.32, 0.26, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.8, 0.10, 0.05, 0.04, 1);
			cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.00, 0.00, 0.00, 1);
			cairo_set_source (cr, pat);
			cairo_set_line_width(cr,5);

			cairo_stroke(cr);

		case INV_LAMP_DRAW_DATA:

			pat = cairo_pattern_create_radial (xc-1, yc-1, 1.5,
						           xc,  yc, r);
			inv_lamp_colour(widget, value*scale, &rc, &cc);
			cairo_pattern_add_color_stop_rgba (pat, 0.0, cc.R,  cc.G,  cc.B,  1);
			cairo_pattern_add_color_stop_rgba (pat, 0.7, rc.R,  rc.G,  rc.B,  1);
			cairo_pattern_add_color_stop_rgba (pat, 0.9, 0.1, 0.0, 0.0, 1);
			cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.1, 0.0, 0.0, 0);
			cairo_set_source (cr, pat);
			cairo_arc(cr,xc,yc,r,0,2*INV_PI);
			cairo_fill(cr);
			INV_LAMP(widget)->lastValue = value;

			break;


	}

  	cairo_destroy(cr);
}


static void
inv_lamp_destroy(GtkObject *object)
{
	InvLamp *lamp;
	InvLampClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_LAMP(object));

	lamp = INV_LAMP(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static void	
inv_lamp_colour(GtkWidget *widget, float value, struct colour *rc, struct colour *cc)
{
	float f1,f2;
	struct colour *r0;
	struct colour *c0;
	struct colour *r1;
	struct colour *c1;

	if(value <= 0) 
	{
		rc->R=INV_LAMP(widget)->l0_r.R;
		rc->G=INV_LAMP(widget)->l0_r.G;
		rc->B=INV_LAMP(widget)->l0_r.B;

		cc->R=INV_LAMP(widget)->l0_c.R;
		cc->G=INV_LAMP(widget)->l0_c.G;
		cc->B=INV_LAMP(widget)->l0_c.B;	
	} 
	else if (value < 1)
	{
		r0=&(INV_LAMP(widget)->l0_r);
		c0=&(INV_LAMP(widget)->l0_c);

		r1=&(INV_LAMP(widget)->l1_r);
		c1=&(INV_LAMP(widget)->l1_c);

		f1=1-value;
		f2=value;

		rc->R=(f1 * r0->R) + (f2 * r1->R);
		rc->G=(f1 * r0->G) + (f2 * r1->G);
		rc->B=(f1 * r0->B) + (f2 * r1->B);

		cc->R=(f1 * c0->R) + (f2 * c1->R);
		cc->G=(f1 * c0->G) + (f2 * c1->G);
		cc->B=(f1 * c0->B) + (f2 * c1->B);
	}
	else if (value < 2)
	{
		r0=&(INV_LAMP(widget)->l1_r);
		c0=&(INV_LAMP(widget)->l1_c);

		r1=&(INV_LAMP(widget)->l2_r);
		c1=&(INV_LAMP(widget)->l2_c);

		f1=2-value;
		f2=value-1;

		rc->R=(f1 * r0->R) + (f2 * r1->R);
		rc->G=(f1 * r0->G) + (f2 * r1->G);
		rc->B=(f1 * r0->B) + (f2 * r1->B);

		cc->R=(f1 * c0->R) + (f2 * c1->R);
		cc->G=(f1 * c0->G) + (f2 * c1->G);
		cc->B=(f1 * c0->B) + (f2 * c1->B);
	}
	else if (value < 3)
	{
		r0=&(INV_LAMP(widget)->l2_r);
		c0=&(INV_LAMP(widget)->l2_c);

		r1=&(INV_LAMP(widget)->l3_r);
		c1=&(INV_LAMP(widget)->l3_c);

		f1=3-value;
		f2=value-2;

		rc->R=(f1 * r0->R) + (f2 * r1->R);
		rc->G=(f1 * r0->G) + (f2 * r1->G);
		rc->B=(f1 * r0->B) + (f2 * r1->B);

		cc->R=(f1 * c0->R) + (f2 * c1->R);
		cc->G=(f1 * c0->G) + (f2 * c1->G);
		cc->B=(f1 * c0->B) + (f2 * c1->B);
	}
	else if (value < 4)
	{
		r0=&(INV_LAMP(widget)->l3_r);
		c0=&(INV_LAMP(widget)->l3_c);

		r1=&(INV_LAMP(widget)->l4_r);
		c1=&(INV_LAMP(widget)->l4_c);

		f1=4-value;
		f2=value-3;

		rc->R=(f1 * r0->R) + (f2 * r1->R);
		rc->G=(f1 * r0->G) + (f2 * r1->G);
		rc->B=(f1 * r0->B) + (f2 * r1->B);

		cc->R=(f1 * c0->R) + (f2 * c1->R);
		cc->G=(f1 * c0->G) + (f2 * c1->G);
		cc->B=(f1 * c0->B) + (f2 * c1->B);
	}
	else
	{
		rc->R=INV_LAMP(widget)->l4_r.R;
		rc->G=INV_LAMP(widget)->l4_r.G;
		rc->B=INV_LAMP(widget)->l4_r.B;

		cc->R=INV_LAMP(widget)->l4_c.R;
		cc->G=INV_LAMP(widget)->l4_c.G;
		cc->B=INV_LAMP(widget)->l4_c.B;	
	}
}



