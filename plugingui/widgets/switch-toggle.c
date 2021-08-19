/* 

    This widget provides toggle switches

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
#include "switch-toggle.h"
#include "switch-toggle-img_off.xpm"
#include "switch-toggle-img_on.xpm"

static void 	inv_switch_toggle_class_init(InvSwitchToggleClass *klass);
static void 	inv_switch_toggle_init(InvSwitchToggle *switch_toggle);
static void 	inv_switch_toggle_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_switch_toggle_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_switch_toggle_realize(GtkWidget *widget);
static gboolean inv_switch_toggle_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_switch_toggle_paint(GtkWidget *widget, gint mode);
static gboolean	inv_switch_toggle_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean inv_switch_toggle_button_release_event (GtkWidget *widget, GdkEventButton *event);
static void	inv_switch_toggle_destroy(GtkObject *object);


GtkType
inv_switch_toggle_get_type(void)
{
	static GType inv_switch_toggle_type = 0;
	char *name;
	int i;


	if (!inv_switch_toggle_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvSwitchToggleClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_switch_toggle_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvSwitchToggle),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_switch_toggle_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvSwitchToggle-%p-%d",inv_switch_toggle_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_switch_toggle_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_switch_toggle_type;
}

void
inv_switch_toggle_set_bypass(InvSwitchToggle *switch_toggle, gint num)
{
	switch_toggle->bypass = num;
}

void
inv_switch_toggle_toggle(InvSwitchToggle *switch_toggle)
{
	if(switch_toggle->state == INV_SWITCH_TOGGLE_ON) {
		switch_toggle->state = INV_SWITCH_TOGGLE_OFF;
		switch_toggle->value = switch_toggle->off_value;
	} else {
		switch_toggle->state = INV_SWITCH_TOGGLE_ON;
		switch_toggle->value = switch_toggle->on_value;
	}
	if(GTK_WIDGET_REALIZED(switch_toggle))
		inv_switch_toggle_paint(GTK_WIDGET(switch_toggle),INV_SWITCH_TOGGLE_DRAW_DATA);
}

float
inv_switch_toggle_get_value(InvSwitchToggle *switch_toggle)
{
	return switch_toggle->value;
}

void
inv_switch_toggle_set_state(InvSwitchToggle *switch_toggle, gint state)
{
	if(switch_toggle->state != state) {
		switch_toggle->state = state;
		switch(state) {
			case INV_SWITCH_TOGGLE_ON:
				switch_toggle->value = switch_toggle->on_value;
				break;
			case INV_SWITCH_TOGGLE_OFF:
				switch_toggle->value = switch_toggle->off_value;
				break;
		}
		if(GTK_WIDGET_REALIZED(switch_toggle))
			inv_switch_toggle_paint(GTK_WIDGET(switch_toggle),INV_SWITCH_TOGGLE_DRAW_DATA);
	}
}

void inv_switch_toggle_set_value(InvSwitchToggle *switch_toggle, gint state, float value)
{
	switch(state) {
		case INV_SWITCH_TOGGLE_ON:
			switch_toggle->on_value=value;
			break;
		case INV_SWITCH_TOGGLE_OFF:
			switch_toggle->off_value=value;
			break;
	}
}

void inv_switch_toggle_set_colour(InvSwitchToggle *switch_toggle, gint state, float R, float G, float B)
{
	switch(state) {
		case INV_SWITCH_TOGGLE_ON:
			switch_toggle->on.R=R;
			switch_toggle->on.G=G;
			switch_toggle->on.B=B;
			break;
		case INV_SWITCH_TOGGLE_OFF:
			switch_toggle->off.R=R;
			switch_toggle->off.G=G;
			switch_toggle->off.B=B;
			break;
	}
}

void inv_switch_toggle_set_text(InvSwitchToggle *switch_toggle, gint state, const char *text)
{
	switch(state) {
		case INV_SWITCH_TOGGLE_ON:
			strncpy(switch_toggle->on_text,text,14);
			break;
		case INV_SWITCH_TOGGLE_OFF:
			strncpy(switch_toggle->off_text,text,14);
			break;
	}
}

void inv_switch_toggle_set_label(InvSwitchToggle *switch_toggle, const char *text)
{
		strncpy(switch_toggle->label,text,14);
}

void inv_switch_toggle_set_tooltip(InvSwitchToggle *switch_toggle, gchar *tip)
{
	gtk_widget_set_tooltip_markup(GTK_WIDGET(switch_toggle),tip);
}



GtkWidget * inv_switch_toggle_new()
{
	return GTK_WIDGET(gtk_type_new(inv_switch_toggle_get_type()));
}


static void
inv_switch_toggle_class_init(InvSwitchToggleClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_switch_toggle_realize;
	widget_class->size_request = inv_switch_toggle_size_request;
	widget_class->size_allocate = inv_switch_toggle_size_allocate;
	widget_class->expose_event = inv_switch_toggle_expose;

    	widget_class->button_press_event = inv_switch_toggle_button_press_event;
    	widget_class->button_release_event = inv_switch_toggle_button_release_event;

	object_class->destroy = inv_switch_toggle_destroy;
}


static void
inv_switch_toggle_init(InvSwitchToggle *switch_toggle)
{

	switch_toggle->bypass    = INV_PLUGIN_ACTIVE;
	switch_toggle->state     = INV_SWITCH_TOGGLE_OFF;
	switch_toggle->laststate = INV_SWITCH_TOGGLE_OFF;
	switch_toggle->value=0;

	switch_toggle->on_value=1;
	switch_toggle->off_value=0;

	switch_toggle->on.R =0.0;	switch_toggle->on.G =1.0;	switch_toggle->on.B =0.0;
	switch_toggle->off.R =1.0;	switch_toggle->off.G =0.0;	switch_toggle->off.B =0.0;

	strcpy(switch_toggle->on_text,"");
	strcpy(switch_toggle->off_text,"");
	strcpy(switch_toggle->label,"");

     	switch_toggle->img_on=gdk_pixbuf_new_from_xpm_data((const char **)switch_toggle_img_on_xpm);
     	switch_toggle->img_off=gdk_pixbuf_new_from_xpm_data((const char **)switch_toggle_img_off_xpm);

	switch_toggle->font_size=0;

    	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(switch_toggle), GTK_CAN_FOCUS);
}


static void
inv_switch_toggle_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_SWITCH_TOGGLE(widget));
	g_return_if_fail(requisition != NULL);

	if(strlen(INV_SWITCH_TOGGLE(widget)->label)>0) {
		requisition->width = 76;
	} else {
		requisition->width = 64;
	}
	
	requisition->height = 66;
}


static void
inv_switch_toggle_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_SWITCH_TOGGLE(widget));
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
inv_switch_toggle_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_SWITCH_TOGGLE(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	if(strlen(INV_SWITCH_TOGGLE(widget)->label)>0) {
		attributes.width = 76;
	} else {
		attributes.width = 64;
	}
	attributes.height = 66;

	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = gtk_widget_get_events(widget) | 
				GDK_EXPOSURE_MASK | 
				GDK_BUTTON_PRESS_MASK | 
				GDK_BUTTON_RELEASE_MASK;

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
inv_switch_toggle_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_SWITCH_TOGGLE(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_switch_toggle_paint(widget,INV_SWITCH_TOGGLE_DRAW_ALL);

	return FALSE;
}


static void
inv_switch_toggle_paint(GtkWidget *widget, gint mode)
{
	gint 		bypass;
	gint 		state;
	gint 		laststate;
	struct colour	on,off;
	char		*on_text;
	char		*off_text;
	char		*label;
	GdkPixbuf 	*img_on;
	GdkPixbuf 	*img_off;

	gint			i;
	float 			indent,topdent,max,grey;
	char 			character[2];
	cairo_t 		*cr;
	GtkStyle		*style;
	cairo_text_extents_t 	extents;
	cairo_pattern_t 	*pat;

	style = gtk_widget_get_style(widget);

	bypass = INV_SWITCH_TOGGLE(widget)->bypass;
	state = INV_SWITCH_TOGGLE(widget)->state;
	laststate = INV_SWITCH_TOGGLE(widget)->laststate;

	if(bypass==INV_PLUGIN_BYPASS) {
		on.R = (INV_SWITCH_TOGGLE(widget)->on.R + INV_SWITCH_TOGGLE(widget)->on.G + INV_SWITCH_TOGGLE(widget)->on.B)/3;
		on.G = on.R;
		on.B = on.R;
		off.R = (INV_SWITCH_TOGGLE(widget)->off.R + INV_SWITCH_TOGGLE(widget)->off.G + INV_SWITCH_TOGGLE(widget)->off.B)/3;
		off.G = off.R;
		off.B = off.R;
	} else {
		on.R = INV_SWITCH_TOGGLE(widget)->on.R;
		on.G = INV_SWITCH_TOGGLE(widget)->on.G;
		on.B = INV_SWITCH_TOGGLE(widget)->on.B;
		off.R = INV_SWITCH_TOGGLE(widget)->off.R;
		off.G = INV_SWITCH_TOGGLE(widget)->off.G;
		off.B = INV_SWITCH_TOGGLE(widget)->off.B;
	}
	on_text = INV_SWITCH_TOGGLE(widget)->on_text;
	off_text = INV_SWITCH_TOGGLE(widget)->off_text;
	label = INV_SWITCH_TOGGLE(widget)->label;
	img_on = INV_SWITCH_TOGGLE(widget)->img_on;
	img_off = INV_SWITCH_TOGGLE(widget)->img_off;

	cr = gdk_cairo_create(widget->window);

	if(INV_SWITCH_TOGGLE(widget)->font_size==0) {
		INV_SWITCH_TOGGLE(widget)->font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,7.1,7.1,"O");
	}
					

	indent = strlen(label)>0 ? 12.0 : 0.0;

	if(mode==INV_SWITCH_TOGGLE_DRAW_ALL) {

		gdk_cairo_set_source_color(cr,&style->bg[GTK_STATE_NORMAL]);
		cairo_paint(cr);

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);

		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		cairo_move_to(cr, indent, 13);
		cairo_line_to(cr, 63+indent, 13);
		cairo_line_to(cr, 63+indent, 0);
		cairo_move_to(cr, indent, 65);
		cairo_line_to(cr, 63+indent, 65);
		cairo_line_to(cr, 63+indent, 52);
		cairo_stroke(cr);

		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		cairo_move_to(cr, indent, 13);
		cairo_line_to(cr, indent, 0);
		cairo_line_to(cr, 63+indent, 0);
		cairo_move_to(cr, indent, 65);
		cairo_line_to(cr, indent, 52);
		cairo_line_to(cr, 63+indent, 52);
		cairo_stroke(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
		cairo_new_path(cr);

		if(strlen(label)>0) {
			if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
				gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
			} else {
				gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
			}
			cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
			cairo_set_line_width(cr,1);
			cairo_rectangle(cr, 0, 1, 10, 64);
			cairo_stroke(cr);

			cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
			cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
			gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
			cairo_set_font_size(cr,INV_SWITCH_TOGGLE(widget)->font_size);
			topdent=42.0-(8.0*(float)(strlen(label))/2);
			for(i=0; i<strlen(label); i++) {
				character[0]=label[i];
				character[1]='\0';
				cairo_text_extents (cr,character,&extents);
				cairo_move_to(cr,extents.width<=2? 4:2, topdent+((float)i*8.0));
				cairo_show_text(cr,character);
			}
		}
	}

	cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cr,INV_SWITCH_TOGGLE(widget)->font_size);

	if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
	} else {
		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
	}

	switch(state) {
		case INV_SWITCH_TOGGLE_ON:

			max = off.R > off.G ? off.R : off.G;
			max = off.B > max ? off.B : max;
			grey=max/3;

			cairo_set_source_rgb(cr, grey/3, grey/3, grey/3);
			cairo_rectangle(cr, 1+indent, 1, 62, 13);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, grey, grey, grey);
			cairo_text_extents (cr,off_text,&extents);
			cairo_move_to(cr,31+indent-(extents.width/2), 11);
			cairo_show_text(cr,off_text);

			pat = cairo_pattern_create_linear (indent, 0.0,  66.0+indent, 0.0);
			cairo_pattern_add_color_stop_rgba (pat, 0.0, on.R/6, on.G/6, on.B/6, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.3, on.R/3, on.G/3, on.B/3, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.5, on.R/2, on.G/2, on.B/2, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.7, on.R/3, on.G/3, on.B/3, 1);
			cairo_pattern_add_color_stop_rgba (pat, 1.0, on.R/6, on.G/6, on.B/6, 1);
			cairo_set_source (cr, pat);
			cairo_rectangle(cr, 1+indent, 53, 62, 13);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, on.R, on.G, on.B);
			cairo_text_extents (cr,on_text,&extents);
			cairo_move_to(cr,31+indent-(extents.width/2), 63);
			cairo_show_text(cr,on_text);

			cairo_save(cr);
			cairo_arc(cr,32+indent,33.5,12,0,2*INV_PI);
			cairo_clip(cr);

			gdk_cairo_set_source_pixbuf(cr,img_on, 32+indent-12.5, 33.5-12.5);
			cairo_paint(cr);

			cairo_restore(cr);

			break;

		case INV_SWITCH_TOGGLE_OFF:

			pat = cairo_pattern_create_linear (indent, 0.0,  66.0+indent, 0.0);
			cairo_pattern_add_color_stop_rgba (pat, 0.0, off.R/6, off.G/6, off.B/6, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.3, off.R/3, off.G/3, off.B/3, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.5, off.R/2, off.G/2, off.B/2, 1);
			cairo_pattern_add_color_stop_rgba (pat, 0.7, off.R/3, off.G/3, off.B/3, 1);
			cairo_pattern_add_color_stop_rgba (pat, 1.0, off.R/6, off.G/6, off.B/6, 1);
			cairo_set_source (cr, pat);
			cairo_rectangle(cr, 1+indent, 1, 62, 13);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, off.R, off.G, off.B);
			cairo_text_extents (cr,off_text,&extents);
			cairo_move_to(cr,31+indent-(extents.width/2), 11);
			cairo_show_text(cr,off_text);

			max = on.R > on.G ? on.R : on.G;
			max = on.B > max ? on.B : max;
			grey=max/3;

			cairo_set_source_rgb(cr, grey/3, grey/3, grey/3);
			cairo_rectangle(cr, 1+indent, 53, 62, 13);
			cairo_fill(cr);

			cairo_set_source_rgb(cr, grey, grey, grey);
			cairo_text_extents (cr,on_text,&extents);
			cairo_move_to(cr,31+indent-(extents.width)/2, 63);
			cairo_show_text(cr,on_text);

			cairo_save(cr);
			cairo_arc(cr,32+indent,33.5,12,0,2*INV_PI);
			cairo_clip(cr);

			gdk_cairo_set_source_pixbuf(cr,img_off, 32+indent-12.5, 33.5-12.5);
			cairo_paint(cr);

			cairo_restore(cr);

			break;
	}

	cairo_save(cr);

	cairo_move_to(cr,32+indent,50.5);
	for(i=1;i<=6;i++) {
		cairo_line_to(cr,32+indent+17*sin(i*INV_PI/3),33.5+17*cos(i*INV_PI/3));
	}
	cairo_clip(cr);

	pat = cairo_pattern_create_linear (indent, 0.0,  66.0+indent, 64.0);
	cairo_pattern_add_color_stop_rgba (pat, 0.0, 1.00, 1.00, 1.00, 1);
	cairo_pattern_add_color_stop_rgba (pat, 0.32, 0.91, 0.89, 0.83, 1);
	cairo_pattern_add_color_stop_rgba (pat, 0.5, 0.43, 0.32, 0.26, 1);
	cairo_pattern_add_color_stop_rgba (pat, 0.68, 0.10, 0.05, 0.04, 1);
	cairo_pattern_add_color_stop_rgba (pat, 1.0, 0.00, 0.00, 0.00, 1);
	cairo_set_source (cr, pat);
	cairo_set_line_width(cr,5);
	cairo_arc(cr,32+indent,33.5,14.5,0,2*INV_PI);
	cairo_stroke(cr);

	cairo_restore(cr);

  	cairo_destroy(cr);
}

static gboolean 
inv_switch_toggle_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_SWITCH_TOGGLE(widget));
	gtk_widget_set_state(widget,GTK_STATE_ACTIVE);
    	gtk_widget_grab_focus(widget);

	inv_switch_toggle_paint(widget,INV_SWITCH_TOGGLE_DRAW_ALL);

	return TRUE;
}


static gboolean 
inv_switch_toggle_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_SWITCH_TOGGLE(widget));
	gtk_widget_set_state(widget,GTK_STATE_NORMAL);

	inv_switch_toggle_toggle(INV_SWITCH_TOGGLE (widget)); //this will also paint the widget

	return FALSE;  //let the signal in the gui run now
}

static void
inv_switch_toggle_destroy(GtkObject *object)
{
	InvSwitchToggle *switch_toggle;
	InvSwitchToggleClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_SWITCH_TOGGLE(object));

	switch_toggle = INV_SWITCH_TOGGLE(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}




