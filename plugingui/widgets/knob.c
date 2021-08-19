/* 

    This widget provides knobs

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
#include "math.h"
#include "string.h"
#include "widgets.h"
#include "knob.h"
#include "knob-img_small.xpm"
#include "knob-img_medium.xpm"
#include "knob-img_large.xpm"


static void 	inv_knob_class_init(InvKnobClass *klass);
static void 	inv_knob_init(InvKnob *knob);
static void 	inv_knob_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_knob_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_knob_realize(GtkWidget *widget);
static gboolean inv_knob_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_knob_paint(GtkWidget *widget, gint mode);
static void	inv_knob_destroy(GtkObject *object);
static gboolean	inv_knob_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean	inv_knob_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);
static gboolean inv_knob_button_release_event (GtkWidget *widget, GdkEventButton *event);

static void	inv_knob_label(gint mode, char *label, char *units, gint human, float value);
static void	inv_knob_label_pan(char *label, float value, float min, float max);
static float	inv_knob_label_set_dp(float value);
static float    inv_marking_to_value(float mark, gint curve, float min, float max);
static float	inv_value_to_angle(float value, gint curve, float min, float max);
static float	inv_value_from_motion(float x_delta, float y_delta, float current, gint curve, float min, float max);


GtkType
inv_knob_get_type(void)
{
	static GType inv_knob_type = 0;
	char *name;
	int i;


	if (!inv_knob_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvKnobClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_knob_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvKnob),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_knob_init
		};
		for (i = 0; ; i++) {

			name = g_strdup_printf("InvKnob-%p-%d", inv_knob_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_knob_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_knob_type;
}

void
inv_knob_set_bypass(InvKnob *knob, gint num)
{
	knob->bypass = num;
}

void
inv_knob_set_size(InvKnob *knob, gint num)
{
	knob->size = num;
}

void
inv_knob_set_curve(InvKnob *knob, gint num)
{
	knob->curve = num;
}

void
inv_knob_set_markings(InvKnob *knob, gint num)
{
	knob->markings = num;
}

void 
inv_knob_set_custom(InvKnob *knob, gint pos, char *label)
{
	switch(pos) {
		case 0:
			strncpy(knob->clow, label, 9);
			break;
		case 1:
			strncpy(knob->cmid, label, 9);
			break;
		case 2:
			strncpy(knob->chigh, label, 9);
			break;
	}
}

void
inv_knob_set_highlight(InvKnob *knob, gint num)
{
	knob->highlight = num;
}

void
inv_knob_set_human(InvKnob *knob)
{
	knob->human=1;
}


void
inv_knob_set_units(InvKnob *knob, char *units)
{
	strncpy(knob->units, units, 4);
}

void
inv_knob_set_min(InvKnob *knob, float num)
{
	knob->min = num;
}

void
inv_knob_set_max(InvKnob *knob, float num)
{
	knob->max = num;
}

void
inv_knob_set_value(InvKnob *knob, float num)
{
	if(num < knob->min) 
		knob->value = knob->min;
	else if(num > knob->max)
		knob->value = knob->min;
	else
		knob->value = num;
	if(knob->value != knob->lastvalue) {
		if(GTK_WIDGET_REALIZED(knob))
			inv_knob_paint(GTK_WIDGET(knob),INV_KNOB_DRAW_DATA);
	}
}

void 
inv_knob_set_tooltip(InvKnob *knob, gchar *tip)
{
	gtk_widget_set_tooltip_markup(GTK_WIDGET(knob),tip);
}

float
inv_knob_get_value(InvKnob *knob)
{
	return knob->value;
}

GtkWidget * inv_knob_new()
{
	return GTK_WIDGET(gtk_type_new(inv_knob_get_type()));
}


static void
inv_knob_class_init(InvKnobClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_knob_realize;
	widget_class->size_request = inv_knob_size_request;
	widget_class->size_allocate = inv_knob_size_allocate;
	widget_class->expose_event = inv_knob_expose;

    	widget_class->button_press_event = inv_knob_button_press_event;
    	widget_class->motion_notify_event = inv_knob_motion_notify_event;
    	widget_class->button_release_event = inv_knob_button_release_event;

	object_class->destroy = inv_knob_destroy;
}


static void
inv_knob_init(InvKnob *knob)
{
	knob->bypass    = INV_PLUGIN_ACTIVE;
	knob->size      = INV_KNOB_SIZE_MEDIUM;
	knob->curve     = INV_KNOB_CURVE_LINEAR;
	knob->markings  = INV_KNOB_MARKINGS_5;
	knob->highlight = INV_KNOB_HIGHLIGHT_L;
	strcpy(knob->units,"");
	strcpy(knob->clow,"");
	strcpy(knob->cmid,"");
	strcpy(knob->chigh,"");
	knob->human 	= 0;
	knob->min       = 0.0;
	knob->max       = 1.0;
	knob->value     = 0.5;
	knob->lastvalue = 0.5;
	knob->click_x	=0;
	knob->click_y	=0;

     	knob->img_small=gdk_pixbuf_new_from_xpm_data((const char **)knob_img_small_xpm);
     	knob->img_med=gdk_pixbuf_new_from_xpm_data((const char **)knob_img_medium_xpm);
     	knob->img_large=gdk_pixbuf_new_from_xpm_data((const char **)knob_img_large_xpm);

	knob->font_size=0;

    	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(knob), GTK_CAN_FOCUS);
}

static void
inv_knob_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_KNOB(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width  = INV_KNOB (widget)->size+4;
	switch(INV_KNOB (widget)->size) {
		case INV_KNOB_SIZE_SMALL:
			requisition->height = INV_KNOB (widget)->size+50;
			break;
		case INV_KNOB_SIZE_MEDIUM:
			requisition->height = INV_KNOB (widget)->size+56;
			break;
		case INV_KNOB_SIZE_LARGE:
		default:
			requisition->height = INV_KNOB (widget)->size+62;
			break;
	}
	
}


static void
inv_knob_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_KNOB(widget));
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
inv_knob_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_KNOB(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = INV_KNOB (widget)->size + 4;
	switch(INV_KNOB (widget)->size) {
		case INV_KNOB_SIZE_SMALL:
			attributes.height = INV_KNOB (widget)->size + 50;
			break;
		case INV_KNOB_SIZE_MEDIUM:
			attributes.height = INV_KNOB (widget)->size + 56;
			break;
		case INV_KNOB_SIZE_LARGE:
		default:
			attributes.height = INV_KNOB (widget)->size + 62;
			break;
	}
	

	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events(widget) |
				GDK_EXPOSURE_MASK | 
				GDK_BUTTON_PRESS_MASK | 
				GDK_BUTTON_RELEASE_MASK | 
				GDK_BUTTON_MOTION_MASK;

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
inv_knob_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_KNOB(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_knob_paint(widget,INV_KNOB_DRAW_ALL);

	return FALSE;
}


static void
inv_knob_paint(GtkWidget *widget, gint mode)
{
	cairo_t		*cr;
	gint  		bypass;
	gint  		size;
	gint  		curve;
	gint  		markings;
	gint  		highlight;
	gint  		human;
	char		*units;
	char		*clow;
	char		*cmid;
	char		*chigh;
	float		min;
	float		max;
	float 		value,lastvalue;
	GdkPixbuf 	*img;
	GtkStateType	state;
	GtkStyle	*style;
	cairo_pattern_t *pat;

	gint i;
	float xc,yc,r,ll,ls,tb,angle;
	gint fontheight;
	char label[20];
	cairo_text_extents_t extents;

	cr = gdk_cairo_create(widget->window);

	state = GTK_WIDGET_STATE(widget);
	style = gtk_widget_get_style(widget);

	bypass = INV_KNOB(widget)->bypass;
	size = INV_KNOB(widget)->size;
	curve = INV_KNOB(widget)->curve;
	markings = INV_KNOB(widget)->markings;
	highlight = INV_KNOB(widget)->highlight;
	human = INV_KNOB(widget)->human;
	units = INV_KNOB(widget)->units;
	clow = INV_KNOB(widget)->clow;
	cmid = INV_KNOB(widget)->cmid;
	chigh = INV_KNOB(widget)->chigh;
	min = INV_KNOB(widget)->min;
	max = INV_KNOB(widget)->max;
	value = INV_KNOB(widget)->value;
	lastvalue = INV_KNOB(widget)->lastvalue;

	xc=(size/2)+2;
	r=size/2;
	switch(size) {
		case INV_KNOB_SIZE_SMALL:
			yc=(size/2)+19;
			fontheight=5;
			ls=3;
			ll=7;
			tb=11;
			img = INV_KNOB(widget)->img_small;
			break;
		case INV_KNOB_SIZE_MEDIUM:
			yc=(size/2)+22;
			fontheight=6;
			ls=5;
			ll=9;
			tb=12;
			img = INV_KNOB(widget)->img_med;
			break;
		case INV_KNOB_SIZE_LARGE:
		default:
			yc=(size/2)+25;
			fontheight=7;
			ls=7;
			ll=11;
			tb=13;
			img = INV_KNOB(widget)->img_large;
			break;
	}

	if(INV_KNOB(widget)->font_size==0) {
		INV_KNOB(widget)->font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,(double)fontheight+0.1,"0");
	}

	/* sanity check - ardour 2.7 doesn't initialise control values properly */

	if(value < min)
		value=min;
	else if (value > max)
		value=max;

	if(mode==INV_KNOB_DRAW_ALL) {

		gdk_cairo_set_source_color(cr,&style->bg[GTK_STATE_NORMAL]);
		cairo_paint(cr);
		cairo_new_path(cr);

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

		if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
			gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		} else {
			gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		}

		if(markings==INV_KNOB_MARKINGS_3 || markings==INV_KNOB_MARKINGS_4 || markings==INV_KNOB_MARKINGS_5 || markings==INV_KNOB_MARKINGS_CUST12) {
			for(i=0;i<=12;i++)
			{ 
				cairo_move_to(cr,xc+(r-6)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r-6)*cos((INV_PI/3)+(i*INV_PI/9)));
				if(i==0 || i==12) {
					/* bottom L & R */
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(i*INV_PI/9)),yc+r*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(i*INV_PI/9)),yc+r-2);
					cairo_set_line_width(cr,2); 
					cairo_stroke(cr);
				} else if ((i==3 || i==9) && markings==INV_KNOB_MARKINGS_5) {
					/*top L & R for M5 */
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(i*INV_PI/9)),yc+r*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(i*INV_PI/9)),yc-r+2);
					cairo_set_line_width(cr,2); 
					cairo_stroke(cr);
				} else if ((i==4 || i==8) && markings==INV_KNOB_MARKINGS_4) {
					/*top L & R for M4 */
					cairo_line_to(cr,xc+(r+ls)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r+ls)*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_line_to(cr,xc+(r+ls)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r+ls)*cos((INV_PI/3)+(i*INV_PI/9))-(ls+1));
					cairo_set_line_width(cr,2);
					cairo_stroke(cr); 
				} else if (i==6 && markings==INV_KNOB_MARKINGS_5) {
					/*top M5 */
					cairo_line_to(cr,xc+(r+ll)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r+ll)*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_set_line_width(cr,2); 
					cairo_stroke(cr);
				} else if (i==6 && (markings==INV_KNOB_MARKINGS_3 || markings==INV_KNOB_MARKINGS_CUST12)){
					/*top M3 */
					cairo_line_to(cr,xc+(r+ls)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r+ls)*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_set_line_width(cr,2);
					cairo_stroke(cr); 
				} else {
					/* all others */
					cairo_line_to(cr,xc+(r-2)*sin((INV_PI/3)+(i*INV_PI/9)),yc+(r-2)*cos((INV_PI/3)+(i*INV_PI/9)));
					cairo_set_line_width(cr,1.3); 
					cairo_stroke(cr);
				} 
			}
		} else { /* pan, cust10 & 10 */
			for(i=0;i<=10;i++)
			{
				cairo_move_to(cr,xc+(r-6)*sin((INV_PI/3)+(4*i*INV_PI/30)),yc+(r-6)*cos((INV_PI/3)+(4*i*INV_PI/30)));
				if(i==0 || i==10) {
					/* bottom L & R */
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(4*i*INV_PI/30)),yc+r*cos((INV_PI/3)+(4*i*INV_PI/30)));
					cairo_line_to(cr,xc+r*sin((INV_PI/3)+(4*i*INV_PI/30)),yc+r-2);
					cairo_set_line_width(cr,2); 
					cairo_stroke(cr);
				} else if (i==5){
					/*top */
					cairo_line_to(cr,xc+(r+ls)*sin((INV_PI/3)+(4*i*INV_PI/30)),yc+(r+ls)*cos((INV_PI/3)+(4*i*INV_PI/30)));
					cairo_set_line_width(cr,2); 
					cairo_stroke(cr);
				} else {
					/* all others */
					cairo_line_to(cr,xc+(r-2)*sin((INV_PI/3)+(4*i*INV_PI/30)),yc+(r-2)*cos((INV_PI/3)+(4*i*INV_PI/30)));
					cairo_set_line_width(cr,1.3); 
					cairo_stroke(cr);
				}
			}
		}

		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_KNOB(widget)->font_size);
		if(bypass==INV_PLUGIN_BYPASS) {
			gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
		} else {
			gdk_cairo_set_source_color(cr,&style->fg[state]);
		}

		/* bottom left */
		switch(markings)
		{
			case INV_KNOB_MARKINGS_PAN:
				switch(size)
				{
					case INV_KNOB_SIZE_MEDIUM:
					case INV_KNOB_SIZE_LARGE:
						strcpy(label,"Left");
						break;
					case INV_KNOB_SIZE_SMALL:
					default:
						strcpy(label,"L");
						break;
				}
				break;

			case INV_KNOB_MARKINGS_CUST10:
			case INV_KNOB_MARKINGS_CUST12:
				strcpy(label,clow);
				break;

			case INV_KNOB_MARKINGS_3:
			case INV_KNOB_MARKINGS_4:
			case INV_KNOB_MARKINGS_5:
			case INV_KNOB_MARKINGS_10:
				inv_knob_label(0,label, units,human, min);
				break;
		}

		cairo_move_to(cr,1,yc+r+8);
		cairo_show_text(cr,label);

		/* bottom right */
		switch(markings)
		{
			case INV_KNOB_MARKINGS_PAN:
				switch(size)
				{
					case INV_KNOB_SIZE_MEDIUM:
					case INV_KNOB_SIZE_LARGE:
						strcpy(label,"Right");
						break;
					case INV_KNOB_SIZE_SMALL:
					default:
						strcpy(label,"R");
						break;
				}
				break;

			case INV_KNOB_MARKINGS_CUST10:
			case INV_KNOB_MARKINGS_CUST12:
				strcpy(label,chigh);
				break;

			case INV_KNOB_MARKINGS_3:
			case INV_KNOB_MARKINGS_4:
			case INV_KNOB_MARKINGS_5:
			case INV_KNOB_MARKINGS_10:
				inv_knob_label(0,label, units, human, max);
				break;
		}

		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,size+1-extents.width,yc+r+8);
		cairo_show_text(cr,label);

		/* top */
		if(markings != INV_KNOB_MARKINGS_4) {
			switch(markings)
			{
				case INV_KNOB_MARKINGS_PAN:
					strcpy(label,"Centre");
					break;

				case INV_KNOB_MARKINGS_CUST10:
				case INV_KNOB_MARKINGS_CUST12:
					strcpy(label,cmid);
					break;

				case INV_KNOB_MARKINGS_3:
				case INV_KNOB_MARKINGS_5:
				case INV_KNOB_MARKINGS_10:
					inv_knob_label(0,label, units, human, inv_marking_to_value(1.0/2.0, curve, min, max));
					break;
				
			}
			cairo_text_extents (cr,label,&extents);
			switch(markings)
			{
				case INV_KNOB_MARKINGS_PAN:
				case INV_KNOB_MARKINGS_3:
				case INV_KNOB_MARKINGS_10:
				case INV_KNOB_MARKINGS_CUST10:
				case INV_KNOB_MARKINGS_CUST12:
					cairo_move_to(cr,xc-(extents.width/2)-1,(2*fontheight)+1);
					break;
				case INV_KNOB_MARKINGS_5:
					cairo_move_to(cr,xc-(extents.width/2)-1,fontheight+3);
					break;
			}
			cairo_show_text(cr,label);
		}

		/* M5 top left */
		if(markings==INV_KNOB_MARKINGS_5) {
			inv_knob_label(0,label, units,human, inv_marking_to_value(1.0/4.0, curve, min, max));
			cairo_move_to(cr,1,yc-r-1);
			cairo_show_text(cr,label);
		}

		/* M5 top right */
		if(markings==INV_KNOB_MARKINGS_5) {
			inv_knob_label(0,label, units,human, inv_marking_to_value(3.0/4.0, curve, min, max));
			cairo_text_extents (cr,label,&extents);
			cairo_move_to(cr,size+1-extents.width,yc-r-1);
			cairo_show_text(cr,label);
		}

		/* M4 top left */
		if(markings==INV_KNOB_MARKINGS_4) {
			inv_knob_label(0,label, units,human, inv_marking_to_value(1.0/3.0, curve, min, max));
			cairo_move_to(cr,1,yc-r-(ls+1));
			cairo_show_text(cr,label);
		}

		/* M4 top right */
		if(markings==INV_KNOB_MARKINGS_4) {
			inv_knob_label(0,label, units,human, inv_marking_to_value(2.0/3.0, curve, min, max));
			cairo_text_extents (cr,label,&extents);
			cairo_move_to(cr,size+1-extents.width,yc-r-(ls+1));
			cairo_show_text(cr,label);
		}
		cairo_new_path(cr);

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);

		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 3, yc+r+ll+tb+10);
		cairo_line_to(cr, 3, yc+r+ll+8);
		cairo_line_to(cr, 2*r+1, yc+r+ll+8);
		cairo_stroke(cr);

		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 3, yc+r+ll+tb+10);
		cairo_line_to(cr, 2*r+1, yc+r+ll+tb+10);
		cairo_line_to(cr, 2*r+1, yc+r+ll+8);
		cairo_stroke(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
		cairo_new_path(cr);

	}

	if(value!=lastvalue || mode==INV_KNOB_DRAW_ALL)
	{
		if(bypass==INV_PLUGIN_BYPASS) {
			gdk_cairo_set_source_color(cr,&style->base[GTK_STATE_INSENSITIVE]);
		} else {
			gdk_cairo_set_source_color(cr,&style->base[state]);
		}
		cairo_rectangle(cr, 4, yc+r+ll+9, 2*r-4, tb);
		cairo_fill(cr);

		cairo_set_font_size(cr,INV_KNOB(widget)->font_size);
		if(bypass==INV_PLUGIN_BYPASS) {
			gdk_cairo_set_source_color(cr,&style->text[GTK_STATE_INSENSITIVE]);
		} else {
			gdk_cairo_set_source_color(cr,&style->text[state]);
		}

		switch(markings) {
			case INV_KNOB_MARKINGS_3:
			case INV_KNOB_MARKINGS_4:	
			case INV_KNOB_MARKINGS_5:	
			case INV_KNOB_MARKINGS_10:
			case INV_KNOB_MARKINGS_CUST10:  
			case INV_KNOB_MARKINGS_CUST12: 
				inv_knob_label(1,label, units, human, value);
				break;
			case INV_KNOB_MARKINGS_PAN:
				inv_knob_label_pan(label, value, min, max);
				break;
		}
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,xc-(extents.width/2)-1,yc+r+ll+11-extents.y_bearing);
		cairo_show_text(cr,label);

		cairo_new_path(cr);


		cairo_set_line_width(cr,1);
		gdk_cairo_set_source_color(cr,&style->bg[GTK_STATE_NORMAL]);
		cairo_arc(cr,xc,yc,r-7.5,0,2*INV_PI);
		cairo_stroke(cr);

		cairo_save(cr);

		angle=inv_value_to_angle(value,curve,min,max);
		cairo_translate(cr,xc,yc);
		cairo_rotate(cr,angle+0.03);
		cairo_arc(cr,0,0,r-9,0,2*INV_PI);
		cairo_clip(cr);
		gdk_cairo_set_source_pixbuf(cr,img, -(r-9), -(r-9));
		cairo_paint(cr);

		cairo_restore(cr);

		pat = cairo_pattern_create_linear (0.0, 0.0,  xc*2, yc*2);
		cairo_pattern_add_color_stop_rgba (pat, 0.0, 0.00, 0.00, 1.00, 1);
		cairo_pattern_add_color_stop_rgba (pat, 0.32, 0.91, 0.89, 0.83, 1);
		cairo_pattern_add_color_stop_rgba (pat, 0.5, 0.43, 0.32, 0.26, 1);
		cairo_pattern_add_color_stop_rgba (pat, 0.68, 0.10, 0.05, 0.04, 1);
		cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.00, 0.00, 1.00, 1);
		cairo_set_source (cr, pat);
		cairo_set_line_width(cr,2.0);
		cairo_arc(cr,xc,yc,r-8.5,0,2*INV_PI);

		cairo_stroke(cr);

		INV_KNOB(widget)->lastvalue=value;
	}
	cairo_destroy(cr);
	
}


static void
inv_knob_destroy(GtkObject *object)
{
	InvKnob *knob;
	InvKnobClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_KNOB(object));

	knob = INV_KNOB(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static gboolean 
inv_knob_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_KNOB(widget));
	g_object_set(G_OBJECT(widget),"has-tooltip",FALSE,NULL);
	gtk_widget_set_state(widget,GTK_STATE_ACTIVE);
    	gtk_widget_grab_focus(widget);

	INV_KNOB(widget)->click_x=event->x;
	INV_KNOB(widget)->click_y=event->y;

	inv_knob_paint(widget,INV_KNOB_DRAW_ALL);

	return TRUE;
}

static gboolean	
inv_knob_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	g_assert(INV_IS_KNOB(widget));

	if((GTK_WIDGET (widget)->state)==GTK_STATE_ACTIVE) {
		INV_KNOB(widget)->value = inv_value_from_motion(INV_KNOB(widget)->click_x-event->x, 
								INV_KNOB(widget)->click_y-event->y, 
								INV_KNOB(widget)->value, 
								INV_KNOB(widget)->curve, 
								INV_KNOB(widget)->min, 
								INV_KNOB(widget)->max );
		INV_KNOB(widget)->click_y = event->y; 
		inv_knob_paint(widget,INV_KNOB_DRAW_DATA);
		return FALSE; //let the after signal run
	} else {
		return TRUE;
	}
}

static gboolean 
inv_knob_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_KNOB(widget));
	gtk_widget_set_state(widget,GTK_STATE_NORMAL);
	g_object_set(G_OBJECT(widget),"has-tooltip",TRUE,NULL);
	inv_knob_paint(widget,INV_KNOB_DRAW_ALL);

	return TRUE;
}

static void
inv_knob_label(gint mode, char *label, char *units, gint human, float value)
{
	float rounded;
	if(mode==0) {
		if(human==1) {
			if(fabs(value)<0.001) {
				sprintf(label,"%0.0fµ%s",value*1000000,units);
			} else if(fabs(value)<1) {
				sprintf(label,"%0.0fm%s",value*1000,units);
			} else if(fabs(value<1000)) {
				sprintf(label,"%0.0f%s",value,units);
			} else if(fabs(value<1000000)) {
				sprintf(label,"%0.0fk%s",value/1000,units);
			} else {
				sprintf(label,"%0.0fM%s",value/1000000,units);
			}
		} else {
			sprintf(label,"%0.0f%s",value,units);
		}
	} else {
		if(human==1) {
			if(fabs(value)<0.001) {
				rounded=inv_knob_label_set_dp(value*1000000);
				sprintf(label,"%0.3g µ%s",rounded,units);
			} else if(fabs(value)<1) {
				rounded=inv_knob_label_set_dp(value*1000);
				sprintf(label,"%0.3g m%s",rounded,units);
			} else if(fabs(value<1000)) {
				rounded=inv_knob_label_set_dp(value);
				sprintf(label,"%0.3g %s",rounded,units);
			} else if(fabs(value<1000000)) {
				rounded=inv_knob_label_set_dp(value/1000);
				sprintf(label,"%0.3g k%s",rounded,units);
			} else {
				rounded=inv_knob_label_set_dp(value/1000000);
				sprintf(label,"%0.3g M%s",rounded,units);
			}
		} else {
			rounded=inv_knob_label_set_dp(value);
			sprintf(label,"%0.3g %s",rounded,units);
		}
	}
} 

static void
inv_knob_label_pan(char *label, float value, float min, float max)
{
	float center;
	gint pan;

	center = (max+min)/2;

	if(value < center) /* left */
	{
		pan=-100 * (value/(center-min));
		if(pan==0) {
			sprintf(label,"Centre");
		} else {
			sprintf(label,"%i%% L",pan);
		}
	} else { 	/* right */
		pan=100 * (value/(max-center));
		if(pan==0) {
			sprintf(label,"Centre");
		} else {
			sprintf(label,"%i%% R",pan);
		}
	}
}

static float
inv_knob_label_set_dp(float value)
{
	float exponent,newval;

	exponent= value==0 ? 0 : log10(fabs(value));

	if(exponent<1) {
		newval=floor(value*100)/100;
	} else if(exponent<2) {
		newval=floor(value*10)/10;
	} else  {
		newval=floor(value);
	}
	return newval;
}

static float
inv_marking_to_value(float mark, gint curve, float min, float max)
{
	float value;
	
	switch(curve)
	{
		case INV_KNOB_CURVE_LOG:
			value=pow(10,log10(min)+(mark*(log10(max)-log10(min))));
			break;
		case INV_KNOB_CURVE_QUAD:
			value= mark < 0.5 ?
				((min+max)/2) - (pow((2*mark)-1,2) * ((max-min)/2)):
				((min+max)/2) + (pow((2*mark)-1,2) * ((max-min)/2));
			break;
		case INV_KNOB_CURVE_LINEAR:
		default:
			value = min + (max-min) * mark;
			break;
	}
	return value;
}

static float
inv_value_to_angle(float value, gint curve, float min, float max)
{
	float angle;

	switch(curve)
	{
		case INV_KNOB_CURVE_LOG:
			angle=(4.0*INV_PI*(log10(value)-log10(min)))/(3*(log10(max)-log10(min))) ;
			break;
		case INV_KNOB_CURVE_QUAD:
			angle= value < (max+min)/2 ?
				4.0*INV_PI*(1-pow(((min+max)-(2*value))/(max-min),0.5))/6 :
				4.0*INV_PI*(pow(((2*value)-(min+max))/(max-min),0.5)+1)/6 ;
			break;
		case INV_KNOB_CURVE_LINEAR:
		default:
			angle = (4.0*INV_PI*(value-min))/(3*(max-min)) ;
			break;
	}
	return angle;
}


static float
inv_value_from_motion(float x_delta, float y_delta, float current, gint curve, float min, float max)
{
	float sens,value,pos;

	sens=1/(75*(1+fabs(x_delta/10)));

	switch(curve)
	{
		case INV_KNOB_CURVE_LOG:
			value = pow(10,log10(current) + (y_delta * sens * (log10(max)-log10(min))));
			break;
		case INV_KNOB_CURVE_QUAD:
			pos= current < (max+min)/2 ?
				(1-pow(((min+max)-(2*current))/(max-min),0.5))/2 :
				(pow(((2*current)-(min+max))/(max-min),0.5)+1)/2 ;
			value= pos + (y_delta * sens) < 0.5 ?
				((min+max)/2) - (pow((2*(pos + (y_delta * sens)))-1,2) * ((max-min)/2)):
				((min+max)/2) + (pow((2*(pos + (y_delta * sens)))-1,2) * ((max-min)/2));
			break;
		case INV_KNOB_CURVE_LINEAR:
		default:
			value = current + (y_delta * sens * (max-min));
			break;
	}	

	if(value < min) value = min;
	if(value > max) value = max;

	return value;
}



