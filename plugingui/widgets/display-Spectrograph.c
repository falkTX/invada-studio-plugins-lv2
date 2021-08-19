/* 

    This widget provides peak display_specs

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
#include "math.h"
#include "widgets.h"
#include "display-Spectrograph.h"


static void 	inv_display_spec_class_init(InvDisplaySpecClass *klass);
static void 	inv_display_spec_init(InvDisplaySpec *display_spec);
static void 	inv_display_spec_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_display_spec_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_display_spec_realize(GtkWidget *widget);
static gboolean inv_display_spec_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_display_spec_paint(GtkWidget *widget, gint drawmode, gint pos);
static void	inv_display_spec_destroy(GtkObject *object);

static void     inv_display_spec_draw_bar(GtkWidget *widget, cairo_t *cr, gint x, gint y, gint pos, gint lastpos, gint drawmode, gint bypass);
static void	inv_display_spec_colour_tozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led);


GtkType
inv_display_spec_get_type(void)
{
	static GType inv_display_spec_type = 0;
	char *name;
	int i;


	if (!inv_display_spec_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvDisplaySpecClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_display_spec_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvDisplaySpec),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_display_spec_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvDisplaySpec-%p-%d",inv_display_spec_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_display_spec_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_display_spec_type;
}

void
inv_display_spec_set_bypass(InvDisplaySpec *display_spec, gint num)
{
	gint i;
	if(display_spec->bypass != num) {
		display_spec->bypass = num;
		for(i=0;i<31;i++) {
			display_spec->value[i]=-90;
		}
	}
}


void
inv_display_spec_set_value(InvDisplaySpec *display_spec, gint pos, float num)
{
	if(pos >=0 && pos <= 30) {
		display_spec->value[pos]=num;
		if(GTK_WIDGET_REALIZED(display_spec)) {
			inv_display_spec_paint(GTK_WIDGET(display_spec),INV_DISPLAY_SPEC_DRAW_ONE,pos);

		}
	}
}


void inv_display_spec_draw_now(InvDisplaySpec *display_spec, gint mode) {
	if(GTK_WIDGET_REALIZED(display_spec)) {
		switch(mode) {
			case INV_DISPLAY_SPEC_DRAW_ALL:
				inv_display_spec_paint(GTK_WIDGET(display_spec),INV_DISPLAY_SPEC_DRAW_ALL,0);
			break;
			case INV_DISPLAY_SPEC_DRAW_DATA:
				inv_display_spec_paint(GTK_WIDGET(display_spec),INV_DISPLAY_SPEC_DRAW_DATA,0);
			break;
		}
	}
}


GtkWidget * inv_display_spec_new()
{
	return GTK_WIDGET(gtk_type_new(inv_display_spec_get_type()));
}


static void
inv_display_spec_class_init(InvDisplaySpecClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_display_spec_realize;
	widget_class->size_request = inv_display_spec_size_request;
	widget_class->size_allocate = inv_display_spec_size_allocate;
	widget_class->expose_event = inv_display_spec_expose;

	object_class->destroy = inv_display_spec_destroy;
}


static void
inv_display_spec_init(InvDisplaySpec *display_spec)
{
	gint i;

	display_spec->bypass = INV_PLUGIN_ACTIVE;

	for(i=0;i<31;i++) {
		display_spec->value[i] = -90.0;
		display_spec->lastvalue[i] = 0;
	}

	strcpy(display_spec->label[0],"20");
	strcpy(display_spec->label[1],"25");
	strcpy(display_spec->label[2],"31");
	strcpy(display_spec->label[3],"40");
	strcpy(display_spec->label[4],"50");
	strcpy(display_spec->label[5],"63");
	strcpy(display_spec->label[6],"80");
	strcpy(display_spec->label[7],"100");
	strcpy(display_spec->label[8],"125");
	strcpy(display_spec->label[9],"160");
	strcpy(display_spec->label[10],"200");
	strcpy(display_spec->label[11],"250");
	strcpy(display_spec->label[12],"315");
	strcpy(display_spec->label[13],"400");
	strcpy(display_spec->label[14],"500");
	strcpy(display_spec->label[15],"630");
	strcpy(display_spec->label[16],"800");
	strcpy(display_spec->label[17],"1k");
	strcpy(display_spec->label[18],"1.2k");
	strcpy(display_spec->label[19],"1.6k");
	strcpy(display_spec->label[20],"2k");
	strcpy(display_spec->label[21],"2.5k");
	strcpy(display_spec->label[22],"3.1k");
	strcpy(display_spec->label[23],"4k");
	strcpy(display_spec->label[24],"5k");
	strcpy(display_spec->label[25],"6.3k");
	strcpy(display_spec->label[26],"8k");
	strcpy(display_spec->label[27],"10k");
	strcpy(display_spec->label[28],"12k");
	strcpy(display_spec->label[29],"16k");
	strcpy(display_spec->label[30],"20k");

	display_spec->mOff60.R =0.1;	display_spec->mOff60.G =0.1;	display_spec->mOff60.B =0.4;
	display_spec->mOn60.R  =-0.1;	display_spec->mOn60.G  =-0.1;	display_spec->mOn60.B  =0.6;

	display_spec->mOff12.R =0.2; 	display_spec->mOff12.G =0.3;	display_spec->mOff12.B =0.4;
	display_spec->mOn12.R  =-0.1;	display_spec->mOn12.G  =0.3;	display_spec->mOn12.B  =0.6;

	display_spec->mOff6.R =0.2; 	display_spec->mOff6.G =0.4;	display_spec->mOff6.B =0.2;
	display_spec->mOn6.R  =0.1;	display_spec->mOn6.G  =0.6;	display_spec->mOn6.B  =-0.1;

	display_spec->mOff0.R  =0.5;	display_spec->mOff0.G  =0.5;	display_spec->mOff0.B  =0.0;
	display_spec->mOn0.R   =0.5;	display_spec->mOn0.G   =0.5;	display_spec->mOn0.B   =0.0;

	display_spec->overOff.R=0.4;	display_spec->overOff.G=0.2;	display_spec->overOff.B=0.0;
	display_spec->overOn.R =0.6;	display_spec->overOn.G =0.0;	display_spec->overOn.B =0.0;

	display_spec->font_size=0;

	gtk_widget_set_tooltip_markup(GTK_WIDGET(display_spec),"<span size=\"8000\">Spectrograph</span>");
}


static void
inv_display_spec_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_SPEC(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 377;
	requisition->height = 160;
}


static void
inv_display_spec_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_SPEC(widget));
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
inv_display_spec_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_SPEC(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;

	attributes.width = 377;
	attributes.height = 160;

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
inv_display_spec_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_DISPLAY_SPEC(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_display_spec_paint(widget,INV_DISPLAY_SPEC_DRAW_ALL,0);

	return FALSE;
}


static void
inv_display_spec_paint(GtkWidget *widget, gint drawmode, gint pos)
{
	gint 		bypass;

	gint 		ledpos,i,fh;
	cairo_t 	*cr;
	GtkStyle	*style;
	char 		label[10];
	cairo_text_extents_t extents;

	style = gtk_widget_get_style(widget);
	bypass = INV_DISPLAY_SPEC(widget)->bypass;


	cr = gdk_cairo_create(widget->window);


	if(INV_DISPLAY_SPEC(widget)->font_size==0) {
		INV_DISPLAY_SPEC(widget)->font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}

	switch(drawmode) {
		case INV_DISPLAY_SPEC_DRAW_ALL:

			gdk_cairo_set_source_color(cr,&style->bg[GTK_STATE_NORMAL]);
			cairo_paint(cr);

			cairo_set_source_rgb(cr, 0, 0, 0);
			cairo_rectangle(cr, 0, 0, 376, 139);
			cairo_fill(cr);

			cairo_new_path(cr);

			cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
			cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
			cairo_set_line_width(cr,1);

			gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
			cairo_move_to(cr, 0, 138);
			cairo_line_to(cr, 0, 0);
			cairo_line_to(cr, 375, 0);
			cairo_stroke(cr);

			gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
			cairo_move_to(cr, 0, 138);
			cairo_line_to(cr, 375, 138);
			cairo_line_to(cr, 375, 0);
			cairo_stroke(cr);


			cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
			cairo_new_path(cr);

			if(bypass==INV_PLUGIN_BYPASS) {
				gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
			} else {
				gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
			}


			cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
			cairo_set_font_size(cr,INV_DISPLAY_SPEC(widget)->font_size);
			strcpy(label,"0");
			cairo_text_extents (cr,label,&extents);
			fh=extents.height;
			for(i=0;i<31;i++) {
				cairo_text_extents (cr,INV_DISPLAY_SPEC(widget)->label[i],&extents);
				switch (i) {
					case 0:
					case 2:
					case 4:
					case 6:
					case 8:
					case 10:
					case 12:
					case 14:
					case 16:
					case 18:
					case 20:
					case 22:
					case 24:
					case 26:
					case 28:
					case 30:
						if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
							gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
						} else {
							gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
						}
						cairo_rectangle(cr, (i*12)+7,140, 1, 2);
						cairo_fill(cr);

						if(bypass==INV_PLUGIN_BYPASS) {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
						} else {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
						}
						cairo_move_to(cr,(i*12)+7-(extents.width/2),144+fh);
						cairo_show_text(cr,INV_DISPLAY_SPEC(widget)->label[i]);
						break;
					case 1:
					case 3:
					case 5:
					case 7:
					case 9:
					case 11:
					case 13:
					case 15:
					case 17:
					case 19:
					case 21:
					case 23:
					case 25:
					case 27:
					case 29:
						if(inv_choose_light_dark(&style->bg[GTK_STATE_NORMAL],&style->light[GTK_STATE_NORMAL],&style->dark[GTK_STATE_NORMAL])==1) {
							gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
						} else {
							gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
						}
						cairo_rectangle(cr, (i*12)+7,140, 1, 11);
						cairo_fill(cr);

						if(bypass==INV_PLUGIN_BYPASS) {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_INSENSITIVE]);
						} else {
							gdk_cairo_set_source_color(cr,&style->fg[GTK_STATE_NORMAL]);
						}
						cairo_move_to(cr,(i*12)+7-(extents.width/2),153+fh);
						cairo_show_text(cr,INV_DISPLAY_SPEC(widget)->label[i]);
						break;
				}
			}

			for(i=0;i<31;i++) {
				ledpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(INV_DISPLAY_SPEC(widget)->value[i]+60.51) : 0 ;
				inv_display_spec_draw_bar(widget, cr, (i*12)+3,137,ledpos,INV_DISPLAY_SPEC(widget)->lastvalue[i], drawmode,bypass);
				INV_DISPLAY_SPEC(widget)->lastvalue[i]=ledpos;
			}
			break;
		case INV_DISPLAY_SPEC_DRAW_DATA:
			for(i=0;i<31;i++) {
				ledpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(INV_DISPLAY_SPEC(widget)->value[i]+60.51) : 0 ;
				inv_display_spec_draw_bar(widget, cr, (i*12)+3,137,ledpos,INV_DISPLAY_SPEC(widget)->lastvalue[i], drawmode,bypass);
				INV_DISPLAY_SPEC(widget)->lastvalue[i]=ledpos;
			}
			break;

		case INV_DISPLAY_SPEC_DRAW_ONE:
			ledpos = bypass==INV_PLUGIN_ACTIVE ? (gint)(INV_DISPLAY_SPEC(widget)->value[pos]+60.51) : 0 ;
			inv_display_spec_draw_bar(widget, cr, (pos*12)+3,137,ledpos,INV_DISPLAY_SPEC(widget)->lastvalue[pos], drawmode,bypass);
			INV_DISPLAY_SPEC(widget)->lastvalue[pos]=ledpos;
			break;

	}
  	cairo_destroy(cr);
}


static void
inv_display_spec_destroy(GtkObject *object)
{
	InvDisplaySpec *display_spec;
	InvDisplaySpecClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_DISPLAY_SPEC(object));

	display_spec = INV_DISPLAY_SPEC(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static void
inv_display_spec_draw_bar(GtkWidget *widget, cairo_t *cr, gint x, gint y, gint pos, gint lastpos, gint drawmode, gint bypass) {

	gint 		i,Lon,min,max;
	struct colour	led;

	switch(drawmode) {
		case INV_DISPLAY_SPEC_DRAW_ALL:
			for ( i = 1; i <= 67; i++) 
			{
				Lon = i <= pos ? 1 : 0;

				inv_display_spec_colour_tozero(widget, bypass, i, Lon, &led);
				cairo_set_source_rgb(cr, led.R, led.G, led.B);
				cairo_rectangle(cr, x, y-(i*2), 10, 1);
				cairo_fill(cr);

			}
			break;
		case INV_DISPLAY_SPEC_DRAW_DATA:
		case INV_DISPLAY_SPEC_DRAW_ONE:
			min = lastpos < pos ? lastpos : pos;
			max = lastpos > pos ? lastpos : pos;
			if(min<1) min=1;
			if(max<1) max=1;
			if(min>67) min=67;
			if(max>67) max=67;
			if(min != max || max == 1 ) {
				for ( i = min ; i <= max; i++) 
				{
					Lon = i <= pos ? 1 : 0;

					inv_display_spec_colour_tozero(widget, bypass, i, Lon, &led);
					cairo_set_source_rgb(cr, led.R, led.G, led.B);
					cairo_rectangle(cr, x, y-(i*2), 10, 1);
					cairo_fill(cr);
				}
			}
			break;
	}
}


static void	
inv_display_spec_colour_tozero(GtkWidget *widget, gint bypass, gint pos, gint on, struct colour *led)
{
	float r1,r2;
	struct colour mOff60  = INV_DISPLAY_SPEC(widget)->mOff60;
	struct colour mOn60   = INV_DISPLAY_SPEC(widget)->mOn60;
	struct colour mOff12  = INV_DISPLAY_SPEC(widget)->mOff12;
	struct colour mOn12   = INV_DISPLAY_SPEC(widget)->mOn12;
	struct colour mOff6   = INV_DISPLAY_SPEC(widget)->mOff6;
	struct colour mOn6    = INV_DISPLAY_SPEC(widget)->mOn6;
	struct colour mOff0   = INV_DISPLAY_SPEC(widget)->mOff0;
	struct colour mOn0    = INV_DISPLAY_SPEC(widget)->mOn0;
	struct colour overOff = INV_DISPLAY_SPEC(widget)->overOff;
	struct colour overOn  = INV_DISPLAY_SPEC(widget)->overOn;
/* 
	66 =  +6dB
	60 =   0dB
	48 = -12dB
	36 = -24dB
*/
	if(pos < 36) 
	{
		r1=(36.0-(float)pos)/36.0;
		r2=(float)pos/36.0;
		led->R=(r1 * mOff60.R + (r2 * mOff12.R))  + (on * ((r1 * mOn60.R) + (r2 * mOn12.R))) ;
		led->G=(r1 * mOff60.G + (r2 * mOff12.G))  + (on * ((r1 * mOn60.G) + (r2 * mOn12.G))) ;
		led->B=(r1 * mOff60.B + (r2 * mOff12.B))  + (on * ((r1 * mOn60.B) + (r2 * mOn12.B))) ;
	} 

	else if (pos < 48)
	{
		r1=(48.0-(float)pos)/12.0;
		r2=((float)pos-36.0)/12.0;
		led->R=(r1 * mOff12.R + (r2 * mOff6.R))  + (on * ((r1 * mOn12.R) + (r2 * mOn6.R))) ;
		led->G=(r1 * mOff12.G + (r2 * mOff6.G))  + (on * ((r1 * mOn12.G) + (r2 * mOn6.G))) ;
		led->B=(r1 * mOff12.B + (r2 * mOff6.B))  + (on * ((r1 * mOn12.B) + (r2 * mOn6.B))) ;
	}

	else if (pos < 60)
	{
		r1=(60.0-(float)pos)/12.0;
		r2=((float)pos-48.0)/12.0;
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


