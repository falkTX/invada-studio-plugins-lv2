/* 

    This widget provides peak meters

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
#include "meter-peak.h"


static void 	inv_meter_class_init(InvMeterClass *klass);
static void 	inv_meter_init(InvMeter *meter);
static void 	inv_meter_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_meter_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_meter_realize(GtkWidget *widget);
static gboolean inv_meter_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_meter_paint(GtkWidget *widget, gint drawmode);
static void	inv_meter_destroy(GtkObject *object);
static void	inv_meter_colour_tozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led);
static void	inv_meter_colour_fromzero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led);
static void	inv_meter_colour_bigtozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led);


GtkType
inv_meter_get_type(void)
{
	static GType inv_meter_type = 0;
	char *name;
	int i;


	if (!inv_meter_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvMeterClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_meter_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvMeter),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_meter_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvMeter-%p-%d",inv_meter_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_meter_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_meter_type;
}

void
inv_meter_set_bypass(InvMeter *meter, gint num)
{
	if(meter->bypass != num) {
		meter->bypass = num;
		switch(meter->mode) {
			case INV_METER_DRAW_MODE_TOZERO:
				meter->LdB=-90;
				meter->RdB=-90;
				break;
			case INV_METER_DRAW_MODE_FROMZERO:
				meter->LdB=0;
				meter->RdB=0;
				break;
		}
	}
}

void
inv_meter_set_channels(InvMeter *meter, gint num)
{
	meter->channels = num;
}

void
inv_meter_set_mode(InvMeter *meter, gint num)
{
	meter->mode = num;
}

void
inv_meter_set_LdB(InvMeter *meter, float num)
{
	meter->LdB = num;
	if(GTK_WIDGET_REALIZED(meter))
		inv_meter_paint(GTK_WIDGET(meter),INV_METER_DRAW_L);
}

void
inv_meter_set_RdB(InvMeter *meter, float num)
{
	meter->RdB = num;
	if(GTK_WIDGET_REALIZED(meter))
		inv_meter_paint(GTK_WIDGET(meter),INV_METER_DRAW_R);
}


GtkWidget * inv_meter_new()
{
	return GTK_WIDGET(gtk_type_new(inv_meter_get_type()));
}


static void
inv_meter_class_init(InvMeterClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_meter_realize;
	widget_class->size_request = inv_meter_size_request;
	widget_class->size_allocate = inv_meter_size_allocate;
	widget_class->expose_event = inv_meter_expose;

	object_class->destroy = inv_meter_destroy;
}


static void
inv_meter_init(InvMeter *meter)
{
	meter->bypass = INV_PLUGIN_ACTIVE;
	meter->mode=INV_METER_DRAW_MODE_TOZERO;
	meter->channels = 1;
	meter->LdB = -90;
	meter->RdB = -90;
	meter->lastLpos = 1;
	meter->lastRpos = 1;


	meter->mOff60.R =0.1;	meter->mOff60.G =0.1;	meter->mOff60.B =0.4;
	meter->mOn60.R  =-0.1;	meter->mOn60.G  =-0.1;	meter->mOn60.B  =0.6;

	meter->mOff12.R =0.2; 	meter->mOff12.G =0.3;	meter->mOff12.B =0.4;
	meter->mOn12.R  =-0.1;	meter->mOn12.G  =0.3;	meter->mOn12.B  =0.6;

	meter->mOff6.R =0.2; 	meter->mOff6.G =0.4;	meter->mOff6.B =0.2;
	meter->mOn6.R  =0.1;	meter->mOn6.G  =0.6;	meter->mOn6.B  =-0.1;

	meter->mOff0.R  =0.5;	meter->mOff0.G  =0.5;	meter->mOff0.B  =0.0;
	meter->mOn0.R   =0.5;	meter->mOn0.G   =0.5;	meter->mOn0.B   =0.0;

	meter->overOff.R=0.4;	meter->overOff.G=0.2;	meter->overOff.B=0.0;
	meter->overOn.R =0.6;	meter->overOn.G =0.0;	meter->overOn.B =0.0;

	meter->label_font_size=0;
	meter->scale_font_size=0;

	gtk_widget_set_tooltip_markup(GTK_WIDGET(meter),"<span size=\"8000\">Peak Meter.</span>");

}


static void
inv_meter_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_METER(widget));
	g_return_if_fail(requisition != NULL);
	switch(INV_METER(widget)->mode) {
		case INV_METER_DRAW_MODE_TOZERO:
		case INV_METER_DRAW_MODE_FROMZERO:
			requisition->width = 149;
			requisition->height = 37;
			break;
		case INV_METER_DRAW_MODE_BIGTOZERO:
			requisition->width = 308;
			requisition->height = 37;
			break;
	}

}


static void
inv_meter_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_METER(widget));
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
inv_meter_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_METER(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;

	switch(INV_METER(widget)->mode) {
		case INV_METER_DRAW_MODE_TOZERO:
		case INV_METER_DRAW_MODE_FROMZERO:
			attributes.width = 149;
			attributes.height = 37;
			break;
		case INV_METER_DRAW_MODE_BIGTOZERO:
			attributes.width = 308;
			attributes.height = 37;
			break;
	}

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
inv_meter_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_METER(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_meter_paint(widget,INV_METER_DRAW_ALL);

	return FALSE;
}


static void
inv_meter_paint(GtkWidget *widget, gint drawmode)
{
	gint 		bypass;
	gint 		mode;
	gint 		channels;
	gint 		Lpos=0;
	gint 		Rpos=0;
	gint 		lastLpos;
	gint 		lastRpos;

	cairo_t 	*cr;
	gint 		Lon,Ron,min,max,i;
	struct colour	led;
	GtkStyle	*style;
	char 		label[10];
	cairo_text_extents_t extents;

	style = gtk_widget_get_style(widget);
	bypass = INV_METER(widget)->bypass;
	mode = INV_METER(widget)->mode;
	channels = INV_METER(widget)->channels;

	switch(mode) {
		case INV_METER_DRAW_MODE_TOZERO:
			Lpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(INV_METER(widget)->LdB+60.51) : 0 ;  /* -60 to +6 db step 1db  = 67 points*/
			Rpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(INV_METER(widget)->RdB+60.51) : 0 ;
			break;
		case INV_METER_DRAW_MODE_FROMZERO:
			Lpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(2*(INV_METER(widget)->LdB)+71.51): 72 ; /* -35.5 to 0 db step 0.5db = 71 points */
			Rpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(2*(INV_METER(widget)->RdB)+71.51): 72 ;
			break;
		case INV_METER_DRAW_MODE_BIGTOZERO:
			Lpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(2*(INV_METER(widget)->LdB)+120.51) : 0 ;  /* -60 to +12 db step 0.5db  = 145 points*/
			Rpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(2*(INV_METER(widget)->RdB)+120.51) : 0 ;
			break;
	}


	lastLpos = INV_METER(widget)->lastLpos;
	lastRpos = INV_METER(widget)->lastRpos;

	cr = gdk_cairo_create(widget->window);

	if(INV_METER(widget)->label_font_size==0) {
		INV_METER(widget)->label_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}
	if(INV_METER(widget)->scale_font_size==0) {
		INV_METER(widget)->scale_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}


	switch(drawmode) {
		case INV_METER_DRAW_ALL:

			gdk_cairo_set_source_color(cr,&style->bg[GTK_STATE_NORMAL]);
			cairo_paint(cr);
			switch(mode) {
				case INV_METER_DRAW_MODE_TOZERO:
				case INV_METER_DRAW_MODE_FROMZERO:
					cairo_set_source_rgb(cr, 0, 0, 0);
					cairo_rectangle(cr, 0, 0, 149, 24);
					cairo_fill(cr);

					cairo_new_path(cr);

					cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
					cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
					cairo_set_line_width(cr,1);

					gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
					cairo_move_to(cr, 0, 23);
					cairo_line_to(cr, 0, 0);
					cairo_line_to(cr, 148, 0);
					cairo_stroke(cr);

					gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
					cairo_move_to(cr, 0, 23);
					cairo_line_to(cr, 148, 23);
					cairo_line_to(cr, 148, 0);
					cairo_stroke(cr);
					break;
				case INV_METER_DRAW_MODE_BIGTOZERO:
					cairo_set_source_rgb(cr, 0, 0, 0);
					cairo_rectangle(cr, 0, 0, 303, 24);
					cairo_fill(cr);

					cairo_new_path(cr);

					cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
					cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
					cairo_set_line_width(cr,1);

					gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
					cairo_move_to(cr, 0, 23);
					cairo_line_to(cr, 0, 0);
					cairo_line_to(cr, 302, 0);
					cairo_stroke(cr);

					gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
					cairo_move_to(cr, 0, 23);
					cairo_line_to(cr, 302, 23);
					cairo_line_to(cr, 302, 0);
					cairo_stroke(cr);
					break;
			}	


			cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
			cairo_new_path(cr);

			cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size(cr,INV_METER(widget)->scale_font_size);

			switch(mode) {
				case INV_METER_DRAW_MODE_TOZERO: 
					for(i=0;i<=5;i++) {
						if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
							gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
						} else {
							gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
						}
						cairo_rectangle(cr, 10+(i*24), 25, 1, 2);
						cairo_fill(cr);

						if(bypass==INV_PLUGIN_BYPASS) {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
						} else {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
						}
						sprintf(label,"%i",(12*i)-60);
						cairo_text_extents (cr,label,&extents);
						cairo_move_to(cr,10+(i*24)-(extents.width/2),35);
						cairo_show_text(cr,label);
					}
					break;

				case INV_METER_DRAW_MODE_FROMZERO:
					for(i=0;i<=5;i++) {
						if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
							gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
						} else {
							gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
						}
						cairo_rectangle(cr, 24+(i*24), 25, 1, 2);
						cairo_fill(cr);

						if(bypass==INV_PLUGIN_BYPASS) {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
						} else {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
						}
						sprintf(label,"%i",30-(6*i));
						cairo_text_extents (cr,label,&extents);
						cairo_move_to(cr,24+(i*24)-(extents.width/2),35);
						cairo_show_text(cr,label);
					}
					break;
				case INV_METER_DRAW_MODE_BIGTOZERO: 
					for(i=0;i<=12;i++) {
						if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
							gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
						} else {
							gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
						}
						cairo_rectangle(cr, 10+(i*24), 25, 1, 2);
						cairo_fill(cr);

						if(bypass==INV_PLUGIN_BYPASS) {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
						} else {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
						}
						if(i>10) {
							sprintf(label,"+%i",(6*i)-60);
						} else {
							sprintf(label,"%i",(6*i)-60);
						}
						cairo_text_extents (cr,label,&extents);
						cairo_move_to(cr,10+(i*24)-(extents.width/2),35);
						cairo_show_text(cr,label);
					}
					break;
			}

			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
			} else {
				cairo_set_source_rgb(cr, 1, 1, 1);
			}
			cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size(cr,INV_METER(widget)->label_font_size);

			switch(mode) {
				case INV_METER_DRAW_MODE_TOZERO:
					switch(channels)
					{
						case 1:
							cairo_move_to(cr,3,15);
							cairo_show_text(cr,"M");
							break;
						case 2:
							cairo_move_to(cr,3,10);
							cairo_show_text(cr,"L");
							cairo_move_to(cr,3,20);
							cairo_show_text(cr,"R");
							break; 
					}

					for ( i = 1; i <= 67; i++) 
					{
						switch(channels)
						{
							case 1:
								Lon = i <= Lpos ? 1 : 0;

								inv_meter_colour_tozero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 3, 1, 18);
								cairo_fill(cr);
								break;
							case 2:
								Lon = i <= Lpos ? 1 : 0;
								Ron = i <= Rpos ? 1 : 0;

								inv_meter_colour_tozero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 3, 1, 8);
								cairo_fill(cr);

								inv_meter_colour_tozero(widget, bypass, i, Ron, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 13, 1, 8);
								cairo_fill(cr);
								break; 
						}
					}
					break;

				case INV_METER_DRAW_MODE_FROMZERO:
					for ( i = 1; i <= 71; i++) 
					{
						switch(channels)
						{
							case 1:
								Lon = i > Lpos ? 1 : 0;

								inv_meter_colour_fromzero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 2+(i*2), 3, 1, 18);
								cairo_fill(cr);
								break;
							case 2:
								Lon = i > Lpos ? 1 : 0;
								Ron = i > Rpos ? 1 : 0;

								inv_meter_colour_fromzero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 2+(i*2), 3, 1, 8);
								cairo_fill(cr);

								inv_meter_colour_fromzero(widget, bypass, i, Ron, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 2+(i*2), 13, 1, 8);
								cairo_fill(cr);
								break; 
						}
					}
					break;
				case INV_METER_DRAW_MODE_BIGTOZERO:
					switch(channels)
					{
						case 1:
							cairo_move_to(cr,3,15);
							cairo_show_text(cr,"M");
							break;
						case 2:
							cairo_move_to(cr,3,10);
							cairo_show_text(cr,"L");
							cairo_move_to(cr,3,20);
							cairo_show_text(cr,"R");
							break; 
					}

					for ( i = 1; i <= 144; i++) 
					{
						switch(channels)
						{
							case 1:
								Lon = i <= Lpos ? 1 : 0;

								inv_meter_colour_bigtozero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 3, 1, 18);
								cairo_fill(cr);
								break;
							case 2:
								Lon = i <= Lpos ? 1 : 0;
								Ron = i <= Rpos ? 1 : 0;

								inv_meter_colour_bigtozero(widget, bypass, i, Lon, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 3, 1, 8);
								cairo_fill(cr);

								inv_meter_colour_bigtozero(widget, bypass, i, Ron, &led);
								cairo_set_source_rgb(cr, led.R, led.G, led.B);
								cairo_rectangle(cr, 10+(i*2), 13, 1, 8);
								cairo_fill(cr);
								break; 
						}
					}
					break;
			}
			INV_METER(widget)->lastLpos = Lpos;
			INV_METER(widget)->lastRpos = Rpos;
			break;

		case INV_METER_DRAW_L:
			switch(mode) {
				case INV_METER_DRAW_MODE_TOZERO:
					min = lastLpos < Lpos ? lastLpos : Lpos;
					max = lastLpos > Lpos ? lastLpos : Lpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>67) min=67;
					if(max>67) max=67;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Lon = i <= Lpos ? 1 : 0;
							inv_meter_colour_tozero(widget, bypass, i, Lon, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							switch(channels)
							{
								case 1:
									cairo_rectangle(cr, 10+(i*2), 3, 1, 18);
									break;
								case 2:
									cairo_rectangle(cr, 10+(i*2), 3, 1, 8);
									break; 
							}
							cairo_fill(cr);
						}
					}
					break;
				case INV_METER_DRAW_MODE_FROMZERO:
					min = lastLpos < Lpos ? lastLpos : Lpos;
					max = lastLpos > Lpos ? lastLpos : Lpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>71) min=71;
					if(max>71) max=71;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Lon = i > Lpos ? 1 : 0;
							inv_meter_colour_fromzero(widget, bypass, i, Lon, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							switch(channels)
							{
								case 1:
									cairo_rectangle(cr, 2+(i*2), 3, 1, 18);
									break;
								case 2:
									cairo_rectangle(cr, 2+(i*2), 3, 1, 8);
									break; 
							}
							cairo_fill(cr);
						}
					}
					break;
				case INV_METER_DRAW_MODE_BIGTOZERO:
					min = lastLpos < Lpos ? lastLpos : Lpos;
					max = lastLpos > Lpos ? lastLpos : Lpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>144) min=144;
					if(max>144) max=144;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Lon = i <= Lpos ? 1 : 0;
							inv_meter_colour_bigtozero(widget, bypass, i, Lon, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							switch(channels)
							{
								case 1:
									cairo_rectangle(cr, 10+(i*2), 3, 1, 18);
									break;
								case 2:
									cairo_rectangle(cr, 10+(i*2), 3, 1, 8);
									break; 
							}
							cairo_fill(cr);
						}
					}
					break;
			}
			INV_METER(widget)->lastLpos = Lpos;
			break;

		case INV_METER_DRAW_R:
			switch(mode) {
				case INV_METER_DRAW_MODE_TOZERO:
					min = lastRpos < Rpos ? lastRpos : Rpos;
					max = lastRpos > Rpos ? lastRpos : Rpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>67) min=67;
					if(max>67) max=67;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Ron = i <= Rpos ? 1 : 0;
							inv_meter_colour_tozero(widget, bypass, i, Ron, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							cairo_rectangle(cr, 10+(i*2), 13, 1, 8);
							cairo_fill(cr);
						}
					}
					break;
				case INV_METER_DRAW_MODE_FROMZERO:
					min = lastRpos < Rpos ? lastRpos : Rpos;
					max = lastRpos > Rpos ? lastRpos : Rpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>71) min=71;
					if(max>71) max=71;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Ron = i > Rpos ? 1 : 0;
							inv_meter_colour_fromzero(widget, bypass, i, Ron, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							cairo_rectangle(cr, 2+(i*2), 13, 1, 8);
							cairo_fill(cr);
						}
					}
					break;
				case INV_METER_DRAW_MODE_BIGTOZERO:
					min = lastRpos < Rpos ? lastRpos : Rpos;
					max = lastRpos > Rpos ? lastRpos : Rpos;
					if(min<1) min=1;
					if(max<1) max=1;
					if(min>144) min=144;
					if(max>144) max=144;
					if(min != max || max == 1 ) {
						for ( i = min ; i <= max; i++) 
						{
							Ron = i <= Rpos ? 1 : 0;
							inv_meter_colour_bigtozero(widget, bypass, i, Ron, &led);
							cairo_set_source_rgb(cr, led.R, led.G, led.B);
							cairo_rectangle(cr, 10+(i*2), 13, 1, 8);
							cairo_fill(cr);
						}
					}
					break;
			}
			INV_METER(widget)->lastRpos = Rpos;
			break;

	}
  	cairo_destroy(cr);
}


static void
inv_meter_destroy(GtkObject *object)
{
	InvMeter *meter;
	InvMeterClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_METER(object));

	meter = INV_METER(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static void	
inv_meter_colour_tozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led)
{
	float r1,r2;
	struct colour mOff60  = INV_METER(widget)->mOff60;
	struct colour mOn60   = INV_METER(widget)->mOn60;
	struct colour mOff12  = INV_METER(widget)->mOff12;
	struct colour mOn12   = INV_METER(widget)->mOn12;
	struct colour mOff6   = INV_METER(widget)->mOff6;
	struct colour mOn6    = INV_METER(widget)->mOn6;
	struct colour mOff0   = INV_METER(widget)->mOff0;
	struct colour mOn0    = INV_METER(widget)->mOn0;
	struct colour overOff = INV_METER(widget)->overOff;
	struct colour overOn  = INV_METER(widget)->overOn;
/* 
	66 =  +6dB
	60 =   0dB
	51 =  -9dB
	42 = -18dB
*/
	if(pos < 42) 
	{
		r1=(42.0-(float)pos)/42.0;
		r2=(float)pos/42.0;
		led->R=(r1 * mOff60.R + (r2 * mOff12.R))  + (on * ((r1 * mOn60.R) + (r2 * mOn12.R))) ;
		led->G=(r1 * mOff60.G + (r2 * mOff12.G))  + (on * ((r1 * mOn60.G) + (r2 * mOn12.G))) ;
		led->B=(r1 * mOff60.B + (r2 * mOff12.B))  + (on * ((r1 * mOn60.B) + (r2 * mOn12.B))) ;
	} 

	else if (pos < 51)
	{
		r1=(51.0-(float)pos)/9.0;
		r2=((float)pos-42.0)/9.0;
		led->R=(r1 * mOff12.R + (r2 * mOff6.R))  + (on * ((r1 * mOn12.R) + (r2 * mOn6.R))) ;
		led->G=(r1 * mOff12.G + (r2 * mOff6.G))  + (on * ((r1 * mOn12.G) + (r2 * mOn6.G))) ;
		led->B=(r1 * mOff12.B + (r2 * mOff6.B))  + (on * ((r1 * mOn12.B) + (r2 * mOn6.B))) ;
	}

	else if (pos < 60)
	{
		r1=(60.0-(float)pos)/9.0;
		r2=((float)pos-51.0)/9.0;
		led->R=(r1 * mOff6.R + (r2 * mOff0.R))  + (on * ((r1 * mOn6.R) + (r2 * mOn0.R))) ;
		led->G=(r1 * mOff6.G + (r2 * mOff0.G))  + (on * ((r1 * mOn6.G) + (r2 * mOn0.G))) ;
		led->B=(r1 * mOff6.B + (r2 * mOff0.B))  + (on * ((r1 * mOn6.B) + (r2 * mOn0.B))) ;
	}
	else
	{
		led->R=overOff.R + (on * overOn.R) ;
		led->G=overOff.G + (on * overOn.G) ;
		led->B=overOff.B + (on * overOn.B) ;
	}	

	if(bypass==INV_PLUGIN_BYPASS) {
		led->R=(led->R+led->G+led->B)/3;
		led->G=led->R;
		led->B=led->R;
	}
}

static void	
inv_meter_colour_fromzero(GtkWidget *widget, gint bypass, gint pos, gint on,  struct colour *led)
{
	float r1,r2;
	struct colour mOff60  = INV_METER(widget)->mOff60;
	struct colour mOn60   = INV_METER(widget)->mOn60;
	struct colour mOff12  = INV_METER(widget)->mOff12;
	struct colour mOn12   = INV_METER(widget)->mOn12;
	struct colour mOff6   = INV_METER(widget)->mOff6;
	struct colour mOn6    = INV_METER(widget)->mOn6;
	struct colour mOff0   = INV_METER(widget)->mOff0;
	struct colour mOn0    = INV_METER(widget)->mOn0;
	struct colour overOff = INV_METER(widget)->overOff;
	struct colour overOn  = INV_METER(widget)->overOn;
/* 
	72 =   0dB
	60 =  -6dB
	48 = -12dB
	24 = -18dB
*/
	if(pos < 24) 
	{
		r1=(24.0-(float)pos)/24.0;
		r2=(float)pos/24.0;
		led->R=(r1 * overOff.R + (r2 * mOff0.R))  + (on * ((r1 * overOn.R) + (r2 * mOn0.R))) ;
		led->G=(r1 * overOff.G + (r2 * mOff0.G))  + (on * ((r1 * overOn.G) + (r2 * mOn0.G))) ;
		led->B=(r1 * overOff.B + (r2 * mOff0.B))  + (on * ((r1 * overOn.B) + (r2 * mOn0.B))) ;
	} 

	else if (pos < 48)
	{
		r1=(48.0-(float)pos)/24.0;
		r2=((float)pos-24.0)/24.0;
		led->R=(r1 * mOff0.R + (r2 * mOff6.R))  + (on * ((r1 * mOn0.R) + (r2 * mOn6.R))) ;
		led->G=(r1 * mOff0.G + (r2 * mOff6.G))  + (on * ((r1 * mOn0.G) + (r2 * mOn6.G))) ;
		led->B=(r1 * mOff0.B + (r2 * mOff6.B))  + (on * ((r1 * mOn0.B) + (r2 * mOn6.B))) ;
	}

	else if (pos < 60)
	{
		r1=(60.0-(float)pos)/12.0;
		r2=((float)pos-48.0)/12.0;
		led->R=(r1 * mOff6.R + (r2 * mOff12.R))  + (on * ((r1 * mOn6.R) + (r2 * mOn12.R))) ;
		led->G=(r1 * mOff6.G + (r2 * mOff12.G))  + (on * ((r1 * mOn6.G) + (r2 * mOn12.G))) ;
		led->B=(r1 * mOff6.B + (r2 * mOff12.B))  + (on * ((r1 * mOn6.B) + (r2 * mOn12.B))) ;
	}
	else if (pos < 72)
	{
		r1=(72.0-(float)pos)/12.0;
		r2=((float)pos-60.0)/12.0;
		led->R=(r1 * mOff12.R + (r2 * mOff60.R))  + (on * ((r1 * mOn12.R) + (r2 * mOn60.R))) ;
		led->G=(r1 * mOff12.G + (r2 * mOff60.G))  + (on * ((r1 * mOn12.G) + (r2 * mOn60.G))) ;
		led->B=(r1 * mOff12.B + (r2 * mOff60.B))  + (on * ((r1 * mOn12.B) + (r2 * mOn60.B))) ;
	}
	else
	{
		led->R=mOff60.R  + (on * mOn60.R) ;
		led->G=mOff60.G  + (on * mOn60.G) ;
		led->B=mOff60.B  + (on * mOn60.B) ;
	}
	if(bypass==INV_PLUGIN_BYPASS) {
		led->R=(led->R+led->G+led->B)/3;
		led->G=led->R;
		led->B=led->R;
	}	
}

static void	
inv_meter_colour_bigtozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led)
{
	float r1,r2;
	struct colour mOff60  = INV_METER(widget)->mOff60;
	struct colour mOn60   = INV_METER(widget)->mOn60;
	struct colour mOff12  = INV_METER(widget)->mOff12;
	struct colour mOn12   = INV_METER(widget)->mOn12;
	struct colour mOff6   = INV_METER(widget)->mOff6;
	struct colour mOn6    = INV_METER(widget)->mOn6;
	struct colour mOff0   = INV_METER(widget)->mOff0;
	struct colour mOn0    = INV_METER(widget)->mOn0;
	struct colour overOff = INV_METER(widget)->overOff;
	struct colour overOn  = INV_METER(widget)->overOn;
/* 
	144 = +12dB
	120 =   0dB
	102 =  -9dB
	84  = -18dB
*/
	if(pos < 84) 
	{
		r1=(84.0-(float)pos)/84.0;
		r2=(float)pos/84.0;
		led->R=(r1 * mOff60.R + (r2 * mOff12.R))  + (on * ((r1 * mOn60.R) + (r2 * mOn12.R))) ;
		led->G=(r1 * mOff60.G + (r2 * mOff12.G))  + (on * ((r1 * mOn60.G) + (r2 * mOn12.G))) ;
		led->B=(r1 * mOff60.B + (r2 * mOff12.B))  + (on * ((r1 * mOn60.B) + (r2 * mOn12.B))) ;
	} 

	else if (pos < 102)
	{
		r1=(102.0-(float)pos)/18.0;
		r2=((float)pos-84.0)/18.0;
		led->R=(r1 * mOff12.R + (r2 * mOff6.R))  + (on * ((r1 * mOn12.R) + (r2 * mOn6.R))) ;
		led->G=(r1 * mOff12.G + (r2 * mOff6.G))  + (on * ((r1 * mOn12.G) + (r2 * mOn6.G))) ;
		led->B=(r1 * mOff12.B + (r2 * mOff6.B))  + (on * ((r1 * mOn12.B) + (r2 * mOn6.B))) ;
	}

	else if (pos < 120)
	{
		r1=(120.0-(float)pos)/18.0;
		r2=((float)pos-102.0)/18.0;
		led->R=(r1 * mOff6.R + (r2 * mOff0.R))  + (on * ((r1 * mOn6.R) + (r2 * mOn0.R))) ;
		led->G=(r1 * mOff6.G + (r2 * mOff0.G))  + (on * ((r1 * mOn6.G) + (r2 * mOn0.G))) ;
		led->B=(r1 * mOff6.B + (r2 * mOff0.B))  + (on * ((r1 * mOn6.B) + (r2 * mOn0.B))) ;
	}
	else
	{
		led->R=overOff.R + (on * overOn.R) ;
		led->G=overOff.G + (on * overOn.G) ;
		led->B=overOff.B + (on * overOn.B) ;
	}	

	if(bypass==INV_PLUGIN_BYPASS) {
		led->R=(led->R+led->G+led->B)/3;
		led->G=led->R;
		led->B=led->R;
	}
}

