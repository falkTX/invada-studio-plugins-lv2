/* 

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
#include <math.h>
#include <string.h>
#include "display-Compressor.h"



static void 	inv_display_comp_class_init(InvDisplayCompClass *klass);
static void 	inv_display_comp_init(InvDisplayComp *displayComp);
static void 	inv_display_comp_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_display_comp_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_display_comp_realize(GtkWidget *widget);
static gboolean inv_display_comp_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_display_comp_paint(GtkWidget *widget, gint mode);
static void	inv_display_comp_destroy(GtkObject *object);
float 		inv_display_comp_rms_waveform(float pos, float width, float height);


GtkType
inv_display_comp_get_type(void)
{
	static GtkType inv_display_comp_type = 0;
	char *name;
	int i;


	if (!inv_display_comp_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvDisplayCompClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_display_comp_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvDisplayComp),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_display_comp_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvDisplayComp-%p-%d",inv_display_comp_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_display_comp_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_display_comp_type;
}

void
inv_display_comp_set_bypass(InvDisplayComp *displayComp, gint num)
{
	displayComp->bypass = num;
}

void
inv_display_comp_set_rms(InvDisplayComp *displayComp, float num)
{
	if(num<0)
		displayComp->rms = 0;
	else if (num <= 1)
		displayComp->rms = num;
	else displayComp->rms = 1;
	if(displayComp->rms != displayComp->Lastrms) {
		if(GTK_WIDGET_REALIZED(displayComp))
			inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}

void
inv_display_comp_set_attack(InvDisplayComp *displayComp, float num)
{
	if(num<0.00001)
		displayComp->attack = 0.00001;
	else if (num <= 0.750)
		displayComp->attack = num;
	else displayComp->attack = 0.750;
	if(displayComp->attack != displayComp->Lastattack) {
		if(GTK_WIDGET_REALIZED(displayComp))
			inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}

void
inv_display_comp_set_release(InvDisplayComp *displayComp, float num)
{
	if(num<0.001)
		displayComp->release = 0.001;
	else if (num <= 5.0)
		displayComp->release = num;
	else displayComp->release = 5.0;
	if(displayComp->release != displayComp->Lastrelease) {
		if(GTK_WIDGET_REALIZED(displayComp))
			inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}

void
inv_display_comp_set_threshold(InvDisplayComp *displayComp, float num)
{
	if(num<-36.0)
		displayComp->threshold = -36.0;
	else if (num <= 0.0)
		displayComp->threshold = num;
	else displayComp->threshold = 0.0;
	if(displayComp->threshold != displayComp->Lastthreshold) {
	if(GTK_WIDGET_REALIZED(displayComp))
		inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}

void
inv_display_comp_set_ratio(InvDisplayComp *displayComp, float num)
{
	if(num<1.0)
		displayComp->ratio = 1.0;
	else if (num <= 20.0)
		displayComp->ratio = num;
	else displayComp->ratio = 20.0;
	if(displayComp->ratio != displayComp->Lastratio) {
		if(GTK_WIDGET_REALIZED(displayComp))
			inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}

void
inv_display_comp_set_gain(InvDisplayComp *displayComp, float num)
{
	if(num<-6.0)
		displayComp->gain = -6.0;
	else if (num <= 36.0)
		displayComp->gain = num;
	else displayComp->gain = 36.0;
	if(displayComp->gain != displayComp->Lastgain) {
		if(GTK_WIDGET_REALIZED(displayComp))
			inv_display_comp_paint(GTK_WIDGET(displayComp),INV_DISPLAYCOMP_DRAW_DATA);
	}
}


GtkWidget * inv_display_comp_new()
{
	return GTK_WIDGET(gtk_type_new(inv_display_comp_get_type()));
}


static void
inv_display_comp_class_init(InvDisplayCompClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_display_comp_realize;
	widget_class->size_request = inv_display_comp_size_request;
	widget_class->size_allocate = inv_display_comp_size_allocate;
	widget_class->expose_event = inv_display_comp_expose;

	object_class->destroy = inv_display_comp_destroy;
}


static void
inv_display_comp_init(InvDisplayComp *displayComp)
{
	gint i;
	displayComp->bypass=INV_PLUGIN_ACTIVE;
	displayComp->rms=0.5;
	displayComp->attack=0.00001;
	displayComp->release=0.001;
	displayComp->threshold=0.0;
	displayComp->ratio=1.0;
	displayComp->gain=0.0;

	displayComp->Lastrms=displayComp->rms;
	displayComp->Lastattack=displayComp->attack;
	displayComp->Lastrelease=displayComp->release;
	displayComp->Lastthreshold=displayComp->threshold;
	displayComp->Lastratio=displayComp->ratio;
	displayComp->Lastgain=displayComp->gain;

	displayComp->SIGmax=0.0;
	for (i=0;i<292;i++) {
		displayComp->SIG[i]=inv_display_comp_rms_waveform((float) i, 292, 104);
		displayComp->SIGmax= fabs(displayComp->SIG[i]) > displayComp->SIGmax ? displayComp->SIG[i] : displayComp->SIGmax;
	}

	displayComp->header_font_size=0;
	displayComp->label_font_size=0;
	displayComp->info_font_size=0;

	gtk_widget_set_tooltip_markup(GTK_WIDGET(displayComp),"<span size=\"8000\"><b>Detector and Envelope:</b> This shows how the RMS, Attack and Release interact to produce an envelope.\n<b>Compressor:</b> This shows how the Threshold, Ratio and Gain affect the audio at different levels.</span>");
}


static void
inv_display_comp_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_COMP(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 600;
	requisition->height = 234;
}


static void
inv_display_comp_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_COMP(widget));
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
inv_display_comp_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_COMP(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = 600;
	attributes.height = 234;

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
inv_display_comp_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_DISPLAY_COMP(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_display_comp_paint(widget,INV_DISPLAYCOMP_DRAW_ALL);

	return FALSE;
}


static void
inv_display_comp_paint(GtkWidget *widget, gint mode)
{
	gint		bypass;
	float 		rms;
	float 		attack;
	float 		release;
	float 		threshold;
	float 		ratio;
	float 		gain;	

	gint		i;
	float		rmsC,rmsV,attackC,releaseC,env;
	float		y,threshsig;
	cairo_t 	*cr;
	GtkStyle	*style;
	char		label[50];

	style = gtk_widget_get_style(widget);
	bypass=INV_DISPLAY_COMP(widget)->bypass;
	rms=INV_DISPLAY_COMP(widget)->rms;
	attack=INV_DISPLAY_COMP(widget)->attack;
	release=INV_DISPLAY_COMP(widget)->release;
	threshold=INV_DISPLAY_COMP(widget)->threshold;
	ratio=INV_DISPLAY_COMP(widget)->ratio;
	gain=INV_DISPLAY_COMP(widget)->gain;

	cr = gdk_cairo_create(widget->window);

	if(INV_DISPLAY_COMP(widget)->header_font_size==0) {
		INV_DISPLAY_COMP(widget)->header_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,8.1,"0");
	}

	if(INV_DISPLAY_COMP(widget)->label_font_size==0) {
		INV_DISPLAY_COMP(widget)->label_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,7.1,"0");
	}

	if(INV_DISPLAY_COMP(widget)->info_font_size==0) {
		INV_DISPLAY_COMP(widget)->info_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}

	if(mode==INV_DISPLAYCOMP_DRAW_ALL) {

		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);

		for(i=0;i<2;i++) {
			gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
			cairo_move_to(cr, (i*300), 233);
			cairo_line_to(cr, (i*300), 0);
			cairo_line_to(cr, (i*300)+299, 0);
			cairo_stroke(cr);

			gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
			cairo_move_to(cr, (i*300), 233);
			cairo_line_to(cr, (i*300)+299, 233);
			cairo_line_to(cr, (i*300)+299, 0);
			cairo_stroke(cr);

			cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
			cairo_new_path(cr);

			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
			} else {
				cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
			}
			cairo_rectangle(cr, (i*300)+1, 1, 298, 232 );
			cairo_fill(cr);
		}

		/* detector labels */
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
		}
		
		cairo_set_font_size(cr,INV_DISPLAY_COMP(widget)->header_font_size);
		sprintf(label,"Detector And Envelope");
		cairo_move_to(cr,75,13);
		cairo_show_text(cr,label);

		cairo_set_font_size(cr,INV_DISPLAY_COMP(widget)->label_font_size);
		sprintf(label,"Audio");
		cairo_move_to(cr,35,230);
		cairo_show_text(cr,label);

		sprintf(label,"Detected Signal");
		cairo_move_to(cr,105,230);
		cairo_show_text(cr,label);

		sprintf(label,"Envelope");
		cairo_move_to(cr,225,230);
		cairo_show_text(cr,label);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.4);
		} else {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
		}


		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 25, 230);
		cairo_line_to(cr, 31, 222);
		cairo_stroke(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.25);
		} else {
			cairo_set_source_rgba(cr, 1.0, 0.1, 0.0, 0.25);
		}
		cairo_rectangle(cr, 95, 222, 6, 6 );
		cairo_fill(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.0, 0.1, 1.0);
		}
		cairo_rectangle(cr, 215, 222, 6, 6 );
		cairo_fill(cr);

		/* compressor labels */
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
		}
		
		cairo_set_font_size(cr,INV_DISPLAY_COMP(widget)->header_font_size);
		sprintf(label,"Compressor");
		cairo_move_to(cr,415,13);
		cairo_show_text(cr,label);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
		} else {
			cairo_set_source_rgb(cr, 1, 1, 1);
		}
		cairo_rectangle(cr, 566, 17, 1, 191);
		cairo_fill(cr);

		cairo_rectangle(cr, 306, 207, 260, 1);
		cairo_fill(cr);

		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_DISPLAY_COMP(widget)->info_font_size);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		}
		for(i=0;i<8;i+=2)
		{
			sprintf(label,"%3idB",-(i*6));
			cairo_move_to(cr,569,42+(i*21));
			cairo_show_text(cr,label);

			sprintf(label,"%idB",-(i*6));
			switch(i) {
				case 0:
					cairo_move_to(cr,530-(i*30),219);
					break;
				case 2:
				case 4:
				case 6:
					cairo_move_to(cr,525-(i*30),219);
					break;
			}
			cairo_show_text(cr,label);
		}

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
		}
		cairo_set_font_size(cr,INV_DISPLAY_COMP(widget)->label_font_size);
		sprintf(label,"Original");
		cairo_move_to(cr,385,230);
		cairo_show_text(cr,label);

		sprintf(label,"Compressed");
		cairo_move_to(cr,465,230);
		cairo_show_text(cr,label);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.4);
		} else {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.4);
		}
		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 375, 230);
		cairo_line_to(cr, 381, 222);
		cairo_stroke(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.0, 0.1, 1);
		}
		cairo_set_line_width(cr,2.0);
		cairo_move_to(cr, 455, 230);
		cairo_line_to(cr, 461, 222);
		cairo_stroke(cr);



	}

	/* rms display */
	if(mode == INV_DISPLAYCOMP_DRAW_ALL 
	|| rms  != INV_DISPLAY_COMP(widget)->Lastrms 
	|| attack    != INV_DISPLAY_COMP(widget)->Lastattack 
	|| release   != INV_DISPLAY_COMP(widget)->Lastrelease ) {


		if(mode == INV_DISPLAYCOMP_DRAW_ALL 
		|| rms  != INV_DISPLAY_COMP(widget)->Lastrms ) {
			//compute new rms if needed
			rmsC= (pow(rms,3) * 400)+1; 
			rmsV=0;
			attackC=  1 - pow(10, -301.0301 / (attack*1000000.0)); 
			releaseC= 1 - pow(10, -301.0301 / (release*1000000.0)); 
			env=0;
			for (i=0;i<292;i++) {
				y=INV_DISPLAY_COMP(widget)->SIG[i];
				rmsV = sqrt(( (rmsC-1)*rmsV*rmsV + y*y ) / rmsC); 
				INV_DISPLAY_COMP(widget)->RMS[i]=rmsV;
				env += (INV_DISPLAY_COMP(widget)->RMS[i] > env) ? 
					attackC  * (INV_DISPLAY_COMP(widget)->RMS[i] - env) : 
					releaseC * (INV_DISPLAY_COMP(widget)->RMS[i] - env) ;
				INV_DISPLAY_COMP(widget)->ENV[i]=env;
			}

		} else if(attack    != INV_DISPLAY_COMP(widget)->Lastattack 
		       || release   != INV_DISPLAY_COMP(widget)->Lastrelease ) {
			//compute new evelope if needed
			attackC=  1 - pow(10, -301.0301 / (attack*1000000.0)); 
			releaseC= 1 - pow(10, -301.0301 / (release*1000000.0)); 
			env=0;
			for (i=0;i<292;i++) {
				env += (INV_DISPLAY_COMP(widget)->RMS[i] > env) ? 
					attackC  * (INV_DISPLAY_COMP(widget)->RMS[i] - env) : 
					releaseC * (INV_DISPLAY_COMP(widget)->RMS[i] - env) ;
				INV_DISPLAY_COMP(widget)->ENV[i]=env;
			}
		}

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}

		cairo_rectangle(cr, 3, 15, 294, 206 );
		cairo_fill(cr);


		//draw envelope
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.0, 0.1, 1);
		}
		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 4, 117);

		for (i=0;i<292;i++) {
			cairo_line_to(cr, 4+i, 117-INV_DISPLAY_COMP(widget)->ENV[i]);
		}
		cairo_line_to(cr, 295, 117);
		cairo_line_to(cr, 4, 117);
		cairo_fill(cr);

		// draw axis
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		} else {
			cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
		}
		cairo_rectangle(cr, 4, 117, 292, 1 );
		cairo_fill(cr);


		//draw rms
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.25);
		} else {
			cairo_set_source_rgba(cr, 1.0, 0.1, 0.0, 0.25);
		}
		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 4, 117);
		for (i=0;i<292;i++) {
			cairo_line_to(cr, 4+i, 117-INV_DISPLAY_COMP(widget)->RMS[i]);
		}
		cairo_line_to(cr, 295, 117);
		cairo_line_to(cr, 4, 117);
		cairo_fill(cr);


		//draw original signal
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.25);
		} else {
			cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
		}
		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 4, 117);
		for (i=0;i<292;i++) {
			cairo_line_to(cr, 4+i, 117-INV_DISPLAY_COMP(widget)->SIG[i]);
		}
		cairo_stroke(cr);
	}


	/* compressor display */
	if(mode      == INV_DISPLAYCOMP_DRAW_ALL 
	|| threshold != INV_DISPLAY_COMP(widget)->Lastthreshold 
	|| ratio     != INV_DISPLAY_COMP(widget)->Lastratio 
	|| gain      != INV_DISPLAY_COMP(widget)->Lastgain ) {

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}
		cairo_rectangle(cr, 306, 17, 260, 190 );
		cairo_fill(cr);

		for(i=1;i<8;i+=2) {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgb(cr, 0.15, 0.15, 0.15);
			} else {
				cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
			}
			cairo_rectangle(cr, 296+(i*30), 17, 1, 190);
			cairo_fill(cr);
			cairo_rectangle(cr, 306, 38+(i*21), 260, 1);
			cairo_fill(cr);
		}

		for(i=0;i<5;i+=2) {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
			} else {
				cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
			}
			cairo_rectangle(cr, 356+(i*30), 17, 1, 190);
			cairo_fill(cr);
			cairo_rectangle(cr, 306, 80+(i*21), 260, 1);
			cairo_fill(cr);
		}


		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		}
		cairo_rectangle(cr, 536, 17, 1, 190);
		cairo_fill(cr);
		cairo_rectangle(cr, 306, 38, 260, 1);
		cairo_fill(cr);

		cairo_rectangle(cr, 306, 17, 260, 190 );
		cairo_clip(cr);

		/* compressed signal */

		// gain change at +6 db <- GRAPH MUST END HERE
		threshsig=pow(10,threshold/20);
		y = 20*log10(threshsig+((2-threshsig)/ratio));

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.0, 0.1, 1);
		}
		cairo_set_line_width(cr,2);

		cairo_move_to(cr, 306                 , 200-(21*gain/6));
		cairo_line_to(cr, 536+(30*threshold/6), 38-(21*gain/6)-(21*threshold/6));
		cairo_line_to(cr, 566                 , 38-(21*gain/6)-(21*y/6)); 
		cairo_stroke(cr);

		/* original signal */
		cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.25);
		cairo_set_line_width(cr,1.0);
		cairo_move_to(cr, 306, 200);
		cairo_line_to(cr, 566, 17);
		cairo_stroke(cr);



	}
  	cairo_destroy(cr);

	INV_DISPLAY_COMP(widget)->Lastrms=rms;
	INV_DISPLAY_COMP(widget)->Lastattack=attack;
	INV_DISPLAY_COMP(widget)->Lastrelease=release;
	INV_DISPLAY_COMP(widget)->Lastthreshold=threshold;
	INV_DISPLAY_COMP(widget)->Lastratio=ratio;
	INV_DISPLAY_COMP(widget)->Lastgain=gain;

}


static void
inv_display_comp_destroy(GtkObject *object)
{
	InvDisplayComp *displayComp;
	InvDisplayCompClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_DISPLAY_COMP(object));

	displayComp = INV_DISPLAY_COMP(object);

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

float 
inv_display_comp_rms_waveform(float pos, float width, float height)
{
  	float theta,theta2;
	float heightr,heightr2;
	float value;
	
	value=0;

	if(pos < width/3) {
		theta=2*INV_PI*(13.5*pow(3*pos/width,0.5));
		heightr=1-(pow(3*pos/width,0.1));
		value+=3*height*heightr*sin(theta);
	}

	if(pos > width/3) {
		theta=2*INV_PI*(8* 3* (pos-width/3) / (2*width) );
		theta2=2*INV_PI*(32* 3* (pos-width/3) / (2*width) );
		heightr=1-(pow(3*(pos-width/3)/(2*width),12));
		heightr2=1-(pow(3*(pos-width/3)/(2*width),0.5));
		value+=0.6*height*heightr*sin(theta) + 0.2*height*heightr2*sin(theta2);
	}
	return value;
}

