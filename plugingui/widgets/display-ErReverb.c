/* 

    This widget provides a display for Early Reflection Reverbs

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
#include "widgets.h"
#include "../../plugin/library/common.h"
#include "display-ErReverb.h"



static void 	inv_display_err_class_init(InvDisplayErrClass *klass);
static void 	inv_display_err_init(InvDisplayErr *displayErr);
static void 	inv_display_err_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void 	inv_display_err_size_allocate(GtkWidget *widget, GtkAllocation *allocation);
static void 	inv_display_err_realize(GtkWidget *widget);
static gboolean inv_display_err_expose(GtkWidget *widget,GdkEventExpose *event);
static void 	inv_display_err_paint(GtkWidget *widget, gint mode);
static void	inv_display_err_destroy(GtkObject *object);
static gboolean inv_display_err_button_press_event (GtkWidget *widget, GdkEventButton *event);
static gboolean	inv_display_err_motion_notify_event(GtkWidget *widget, GdkEventMotion *event);
static gboolean inv_display_err_button_release_event(GtkWidget *widget, GdkEventButton *event);
static void	inv_display_err_screen(struct point3D *point, struct point2D * screen, struct point3D * camera, struct point3D *lookat);
static gint	inv_display_err_find_active_dot(float l, float w, float sLR, float sFB, float dLR, float dFB, float x, float y);
static void	inv_display_err_update_active_dot(gint dot, float l, float w, float x, float y, float *LR, float *FB);


GtkType
inv_display_err_get_type(void)
{
	static GtkType inv_display_err_type = 0;
	char *name;
	int i;


	if (!inv_display_err_type) 
	{
		static const GTypeInfo type_info = {
			sizeof(InvDisplayErrClass),
			NULL, /* base_init */
			NULL, /* base_finalize */
			(GClassInitFunc)inv_display_err_class_init,
			NULL, /* class_finalize */
			NULL, /* class_data */
			sizeof(InvDisplayErr),
			0,    /* n_preallocs */
			(GInstanceInitFunc)inv_display_err_init
		};
		for (i = 0; ; i++) {
			name = g_strdup_printf("InvDisplayErr-%p-%d",inv_display_err_class_init, i);
			if (g_type_from_name(name)) {
				free(name);
				continue;
			}
			inv_display_err_type = g_type_register_static(GTK_TYPE_WIDGET,name,&type_info,(GTypeFlags)0);
			free(name);
			break;
		}
	}
	return inv_display_err_type;
}

gint 
inv_display_err_get_active_dot(InvDisplayErr *displayErr) {

	return displayErr->active_dot;
}

void
inv_display_err_set_bypass(InvDisplayErr *displayErr, gint num)
{
	displayErr->bypass = num;
}

float 
inv_display_err_get_source(InvDisplayErr *displayErr, gint axis) {
	return displayErr->source[axis];
}

float 
inv_display_err_get_dest(InvDisplayErr *displayErr, gint axis){
	return displayErr->dest[axis];
}



void
inv_display_err_set_room(InvDisplayErr *displayErr, gint axis, float num)
{
	switch(axis) {
		case INV_DISPLAY_ERR_ROOM_LENGTH:
		case INV_DISPLAY_ERR_ROOM_WIDTH:
			if(num<3) num=3;
			if(num>100) num=100;
			displayErr->room[axis]=num;
			break;
		case INV_DISPLAY_ERR_ROOM_HEIGHT:
			if(num<3) num=3;
			if(num>30) num=30;
			displayErr->room[axis]=num;
			break;
	}
	if(displayErr->room[axis] != displayErr->Lastroom[axis]) {
		if(GTK_WIDGET_REALIZED(displayErr))
			inv_display_err_paint(GTK_WIDGET(displayErr),INV_DISPLAY_ERR_DRAW_DATA);
	}
}

void
inv_display_err_set_source(InvDisplayErr *displayErr, gint axis, float num)
{
	switch(axis) {
		case INV_DISPLAY_ERR_LR:
			if(num<-1) num=-1;
			if(num>1) num=1;
			displayErr->source[axis]=num;
			break;
		case INV_DISPLAY_ERR_FB:
			if(num<0.51) num=0.51;
			if(num>0.99) num=0.99;
			displayErr->source[axis]=num;
			break;
	}
	if(displayErr->source[axis] != displayErr->Lastsource[axis]) {
		if(GTK_WIDGET_REALIZED(displayErr))
			inv_display_err_paint(GTK_WIDGET(displayErr),INV_DISPLAY_ERR_DRAW_DATA);
	}
}

void
inv_display_err_set_dest(InvDisplayErr *displayErr, gint axis, float num)
{
	switch(axis) {
		case INV_DISPLAY_ERR_LR:
			if(num<-1) num=-1;
			if(num>1) num=1;
			displayErr->dest[axis]=num;
			break;
		case INV_DISPLAY_ERR_FB:
			if(num<0.01) num=0.01;
			if(num>0.49) num=0.49;
			displayErr->dest[axis]=num;
			break;
	}
	if(displayErr->dest[axis] != displayErr->Lastdest[axis]) {
		if(GTK_WIDGET_REALIZED(displayErr))
			inv_display_err_paint(GTK_WIDGET(displayErr),INV_DISPLAY_ERR_DRAW_DATA);
	}
}

void
inv_display_err_set_diffusion(InvDisplayErr *displayErr, float num)
{
	if(num<0) num=0;
	if(num>100) num=1;
	displayErr->diffusion=num/100;

	if(displayErr->diffusion != displayErr->Lastdiffusion) {
		if(GTK_WIDGET_REALIZED(displayErr))
			inv_display_err_paint(GTK_WIDGET(displayErr),INV_DISPLAY_ERR_DRAW_DATA);
	}
}

GtkWidget * inv_display_err_new()
{
	return GTK_WIDGET(gtk_type_new(inv_display_err_get_type()));
}


static void
inv_display_err_class_init(InvDisplayErrClass *klass)
{
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;


	widget_class = (GtkWidgetClass *) klass;
	object_class = (GtkObjectClass *) klass;

	widget_class->realize = inv_display_err_realize;
	widget_class->size_request = inv_display_err_size_request;
	widget_class->size_allocate = inv_display_err_size_allocate;
	widget_class->expose_event = inv_display_err_expose;

    	widget_class->button_press_event = inv_display_err_button_press_event;
    	widget_class->motion_notify_event = inv_display_err_motion_notify_event;
    	widget_class->button_release_event = inv_display_err_button_release_event;

	object_class->destroy = inv_display_err_destroy;
}


static void
inv_display_err_init(InvDisplayErr *displayErr)
{
	displayErr->bypass=INV_PLUGIN_ACTIVE;
	displayErr->active_dot=INV_DISPLAY_ERR_DOT_NONE;

	displayErr->room[INV_DISPLAY_ERR_ROOM_LENGTH]=25.0;
	displayErr->room[INV_DISPLAY_ERR_ROOM_WIDTH]=30.0;
	displayErr->room[INV_DISPLAY_ERR_ROOM_HEIGHT]=10.0;
	displayErr->source[INV_DISPLAY_ERR_LR]=-0.01;
	displayErr->source[INV_DISPLAY_ERR_FB]=0.8;
	displayErr->dest[INV_DISPLAY_ERR_LR]=0.01;
	displayErr->dest[INV_DISPLAY_ERR_FB]=0.2;
	displayErr->diffusion=50.0;

	displayErr->Lastroom[INV_DISPLAY_ERR_ROOM_LENGTH]=25.0;
	displayErr->Lastroom[INV_DISPLAY_ERR_ROOM_WIDTH]=30.0;
	displayErr->Lastroom[INV_DISPLAY_ERR_ROOM_HEIGHT]=10.0;
	displayErr->Lastsource[INV_DISPLAY_ERR_LR]=-0.01;
	displayErr->Lastsource[INV_DISPLAY_ERR_FB]=0.8;
	displayErr->Lastdest[INV_DISPLAY_ERR_LR]=0.01;
	displayErr->Lastdest[INV_DISPLAY_ERR_FB]=0.2;
	displayErr->Lastdiffusion=50.0;

	displayErr->er=(struct ERunit *)malloc(sizeof(struct ERunit) * MAX_ER);
	displayErr->er_size=0;

	displayErr->header_font_size=0;
	displayErr->info_font_size=0;

    	GTK_WIDGET_SET_FLAGS (GTK_WIDGET(displayErr), GTK_CAN_FOCUS);

	gtk_widget_set_tooltip_markup(GTK_WIDGET(displayErr),"<span size=\"8000\"><b>Room Shape:</b> This is a 3D representation of the virtual room.\n<b>Impulse Response:</b> This shows the resultant inpulse response of the room.\n<b>Source and Listerner Position:</b> Use this display to position the source and listener in the virtual room.</span>");
}


static void
inv_display_err_size_request(GtkWidget *widget,
    GtkRequisition *requisition)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_ERR(widget));
	g_return_if_fail(requisition != NULL);

	requisition->width = 510;
	requisition->height = 300;
}


static void
inv_display_err_size_allocate(GtkWidget *widget,
    GtkAllocation *allocation)
{
	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_ERR(widget));
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
inv_display_err_realize(GtkWidget *widget)
{
	GdkWindowAttr attributes;
	guint attributes_mask;

	g_return_if_fail(widget != NULL);
	g_return_if_fail(INV_IS_DISPLAY_ERR(widget));

	GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = widget->allocation.x;
	attributes.y = widget->allocation.y;
	attributes.width = 510;
	attributes.height = 300;

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
inv_display_err_expose(GtkWidget *widget, GdkEventExpose *event)
{
	g_return_val_if_fail(widget != NULL, FALSE);
	g_return_val_if_fail(INV_IS_DISPLAY_ERR(widget), FALSE);
	g_return_val_if_fail(event != NULL, FALSE);

	inv_display_err_paint(widget,INV_DISPLAY_ERR_DRAW_ALL);

	return FALSE;
}


static void
inv_display_err_paint(GtkWidget *widget, gint mode)
{
	gint		active_dot;
	gint 		bypass;
	float 		w,l,h;
	float 		sLR,sFB;
	float 		dLR,dFB;
	float 		diffusion;
	struct ERunit   *er;
	gint		er_size;	

	gint		i;
	float		sw,sl,scale;
	float		x,y,xc,yc,r;
	struct point3D	camera,lookat,source,dest,sourceF,destF;
	struct point2D	sourceS,destS,sourceSF,destSF;
	struct point3D	room[8];
	struct point2D	roomS[8];
	float		minx,maxx,miny,maxy,sx,sy,dist;
	char		label[30];
	float		min_delay,max_delay,max_gain;

	cairo_t 	*cr;
	GtkStyle	*style;
	cairo_text_extents_t extents;

	bypass=INV_DISPLAY_ERR(widget)->bypass;
	active_dot=INV_DISPLAY_ERR(widget)->active_dot;
	l=INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_LENGTH];
	w=INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_WIDTH];
	h=INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_HEIGHT];
	sLR=INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_LR];
	sFB=INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_FB];
	dLR=INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_LR];
	dFB=INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_FB];
	diffusion=INV_DISPLAY_ERR(widget)->diffusion;

	er=INV_DISPLAY_ERR(widget)->er;
	er_size=INV_DISPLAY_ERR(widget)->er_size;

	style = gtk_widget_get_style(widget);
	cr = gdk_cairo_create(widget->window);

	if(INV_DISPLAY_ERR(widget)->header_font_size==0) {
		INV_DISPLAY_ERR(widget)->header_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,8.1,"0");
	}

	if(INV_DISPLAY_ERR(widget)->info_font_size==0) {
		INV_DISPLAY_ERR(widget)->info_font_size=inv_choose_font_size(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL,99.0,6.1,"0");
	}

	if(mode==INV_DISPLAY_ERR_DRAW_ALL) {
		cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER);
		cairo_set_line_width(cr,1);

		for(i=0;i<2;i++) {

			cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
			gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
			cairo_move_to(cr, 0, (i*150)+149);
			cairo_line_to(cr, 0, (i*150));
			cairo_line_to(cr, 209, (i*150));
			cairo_stroke(cr);

			gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
			cairo_move_to(cr, 0, (i*150)+149);
			cairo_line_to(cr, 209, (i*150)+149);
			cairo_line_to(cr, 209, (i*150));
			cairo_stroke(cr);

			cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);

			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
			} else {
				cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
			}
			cairo_rectangle(cr, 1, (i*150)+1, 208, 148 );
			cairo_fill(cr);
		}

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		gdk_cairo_set_source_color(cr,&style->dark[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 210, 299);
		cairo_line_to(cr, 210, 0);
		cairo_line_to(cr, 509, 0);
		cairo_stroke(cr);

		gdk_cairo_set_source_color(cr,&style->light[GTK_STATE_NORMAL]);
		cairo_move_to(cr, 210, 299);
		cairo_line_to(cr, 509, 299);
		cairo_line_to(cr, 509, 0);
		cairo_stroke(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}
		cairo_rectangle(cr, 211, 1, 318, 298 );
		cairo_fill(cr);

		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
		}
		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->header_font_size);
		sprintf(label,"Source And Listener Position");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,360-(extents.width/2),11);
		cairo_show_text(cr,label);

		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->info_font_size);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		}
		sprintf(label,"(click and drag to move)");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,360-(extents.width/2),21);
		cairo_show_text(cr,label);

		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->header_font_size);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
		}

		sprintf(label,"Room Shape");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,105-(extents.width/2),11);
		cairo_show_text(cr,label);
		
		sprintf(label,"Impulse Response");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,105-(extents.width/2),161);
		cairo_show_text(cr,label);
	}


	/* room display */
	/* 200x140 centered at 105,75 */
	if(l!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_LENGTH] || 
	   w!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_WIDTH] ||
	   h!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_HEIGHT] ||
	   sLR!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_LR] ||
	   sFB!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_FB] ||
	   dLR!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_LR] ||
	   dFB!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_FB] ||
	   mode==INV_DISPLAY_ERR_DRAW_ALL) {



		camera.x=4*l;		
		camera.y=(2*h)+((w+l)/2);	
		camera.z=3*w;

		lookat.x=0;		
		lookat.y=0;		
		lookat.z=0;

		dist=log10(pow(camera.x*camera.x+camera.y*camera.y+camera.z+camera.z,0.5));

		source.x=sLR*w/2;	source.y=1.5-h/2;	source.z=((1-sFB)*l)-(l/2);
		sourceF.x=source.x;	sourceF.y=-h/2;		sourceF.z=source.z;

		dest.x=dLR*w/2;		dest.y=1.5-h/2;		dest.z=((1-dFB)*l)-(l/2);
		destF.x=dest.x;		destF.y=-h/2;		destF.z=dest.z;

		room[0].x=-w/2;	room[0].y=h/2;	room[0].z=-l/2;
		room[1].x=w/2;	room[1].y=h/2;	room[1].z=-l/2;
		room[2].x=w/2;	room[2].y=h/2;	room[2].z=l/2;
		room[3].x=-w/2;	room[3].y=h/2;	room[3].z=l/2;
		room[4].x=-w/2;	room[4].y=-h/2;	room[4].z=-l/2;
		room[5].x=w/2;	room[5].y=-h/2;	room[5].z=-l/2;
		room[6].x=w/2;	room[6].y=-h/2;	room[6].z=l/2;
		room[7].x=-w/2;	room[7].y=-h/2;	room[7].z=l/2;

		inv_display_err_screen(&source,&sourceS,&camera,&lookat);
		inv_display_err_screen(&sourceF,&sourceSF,&camera,&lookat);
		inv_display_err_screen(&dest,&destS,&camera,&lookat);
		inv_display_err_screen(&destF,&destSF,&camera,&lookat);

		sx=0;
		sy=0;
		minx=9999999;
		maxx=-9999999;
		miny=9999999;
		maxy=-9999999;
		//project to screen
		for(i=0;i<8;i++) {
			inv_display_err_screen(&room[i],&roomS[i],&camera,&lookat);
			minx = roomS[i].x < minx ? roomS[i].x : minx;
			maxx = roomS[i].x > maxx ? roomS[i].x : maxx;
			miny = roomS[i].y < miny ? roomS[i].y : miny;
			maxy = roomS[i].y > maxy ? roomS[i].y : maxy;
		}
		//center
		sourceS.x=sourceS.x-(minx+maxx)/2;
		sourceS.y=sourceS.y-(miny+maxy)/2;
		sourceSF.x=sourceSF.x-(minx+maxx)/2;
		sourceSF.y=sourceSF.y-(miny+maxy)/2;
		destS.x=destS.x-(minx+maxx)/2;
		destS.y=destS.y-(miny+maxy)/2;
		destSF.x=destSF.x-(minx+maxx)/2;
		destSF.y=destSF.y-(miny+maxy)/2;
		for(i=0;i<8;i++) {
			roomS[i].x=roomS[i].x-(minx+maxx)/2;
			roomS[i].y=roomS[i].y-(miny+maxy)/2;
		}

		//scale
		sx = 93/fabs((maxx-minx)/2);
		sy = 65/fabs((maxy-miny)/2);

		scale = sx > sy ? sy : sx;

		//plot with scale
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}
		cairo_rectangle(cr, 3, 13, 204, 134 );
		cairo_fill(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);


		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_line_width(cr,3-dist);
			cairo_set_source_rgb(cr, 0.18, 0.18, 0.18);
		} else {
			cairo_set_line_width(cr,4-dist);
			cairo_set_source_rgb(cr, 0.0, 0.05, 0.5);
		}
		
		cairo_move_to(cr,105+roomS[4].x*scale,80-roomS[4].y*scale);
		cairo_line_to(cr,105+roomS[5].x*scale,80-roomS[5].y*scale);
		cairo_move_to(cr,105+roomS[4].x*scale,80-roomS[4].y*scale);
		cairo_line_to(cr,105+roomS[0].x*scale,80-roomS[0].y*scale);
		cairo_move_to(cr,105+roomS[4].x*scale,80-roomS[4].y*scale);
		cairo_line_to(cr,105+roomS[7].x*scale,80-roomS[7].y*scale);
		cairo_stroke(cr);

		r=5-dist;
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 0.8, 0.8, 0.0);
		}
		cairo_set_line_width(cr,1);

		cairo_move_to(cr,105+sourceSF.x*scale,80-sourceSF.y*scale);
		cairo_line_to(cr,105+sourceS.x*scale,80-sourceS.y*scale);
		cairo_stroke(cr);
		cairo_arc(cr,105+sourceS.x*scale,80-sourceS.y*scale,r,0,2*INV_PI);
		cairo_fill(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		} else {
			cairo_set_source_rgb(cr, 1.0, 0.1, 0.0);
		}	

		cairo_move_to(cr,105+destSF.x*scale,80-destSF.y*scale);
		cairo_line_to(cr,105+destS.x*scale,80-destS.y*scale);
		cairo_stroke(cr);
		cairo_arc(cr,105+destS.x*scale,80-destS.y*scale,r,0,2*INV_PI);
		cairo_fill(cr);

		
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_line_width(cr,3-dist);
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_line_width(cr,4-dist);
			cairo_set_source_rgb(cr, 0.0, 0.1, 1.0);
		}
		cairo_move_to(cr,105+roomS[0].x*scale,80-roomS[0].y*scale);
		cairo_line_to(cr,105+roomS[1].x*scale,80-roomS[1].y*scale);
		cairo_move_to(cr,105+roomS[1].x*scale,80-roomS[1].y*scale);
		cairo_line_to(cr,105+roomS[2].x*scale,80-roomS[2].y*scale);
		cairo_move_to(cr,105+roomS[2].x*scale,80-roomS[2].y*scale);
		cairo_line_to(cr,105+roomS[3].x*scale,80-roomS[3].y*scale);
		cairo_move_to(cr,105+roomS[3].x*scale,80-roomS[3].y*scale);
		cairo_line_to(cr,105+roomS[0].x*scale,80-roomS[0].y*scale);
		cairo_move_to(cr,105+roomS[5].x*scale,80-roomS[5].y*scale);
		cairo_line_to(cr,105+roomS[6].x*scale,80-roomS[6].y*scale);
		cairo_move_to(cr,105+roomS[6].x*scale,80-roomS[6].y*scale);
		cairo_line_to(cr,105+roomS[7].x*scale,80-roomS[7].y*scale);
		cairo_move_to(cr,105+roomS[1].x*scale,80-roomS[1].y*scale);
		cairo_line_to(cr,105+roomS[5].x*scale,80-roomS[5].y*scale);
		cairo_move_to(cr,105+roomS[2].x*scale,80-roomS[2].y*scale);
		cairo_line_to(cr,105+roomS[6].x*scale,80-roomS[6].y*scale);
		cairo_move_to(cr,105+roomS[3].x*scale,80-roomS[3].y*scale);
		cairo_line_to(cr,105+roomS[7].x*scale,80-roomS[7].y*scale);
		cairo_stroke(cr);
	}

	/* impulse response display */
	/* any change alters this display */
	/* 200x140 centered at 105,225 */

	if(l!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_LENGTH] || 
	   w!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_WIDTH] ||
	   h!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_HEIGHT] ||
	   sLR!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_LR] ||
	   sFB!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_FB] ||
	   dLR!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_LR] ||
	   dFB!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_FB] ||
	   diffusion!=INV_DISPLAY_ERR(widget)->Lastdiffusion ||
	   mode==INV_DISPLAY_ERR_DRAW_ALL) {

		min_delay=9999999;
		max_delay=0;
		max_gain=0;
		er_size=calculateIReverbER(er, MAX_ER, w, l, h, sLR, sFB, dLR, dFB, 1.5, diffusion, SPEED_OF_SOUND);
		for(i=0;i<er_size;i++) {
			min_delay=er->DelayActual < min_delay ? er->DelayActual : min_delay;
			max_delay=er->DelayActual > max_delay ? er->DelayActual : max_delay;
			max_gain=fabs(er->GainL) > max_gain ? fabs(er->GainL) : max_gain;
			max_gain=fabs(er->GainR) > max_gain ? fabs(er->GainR) : max_gain;
			er++;
		}

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}
		cairo_rectangle(cr, 3, 163, 204, 134 );
		cairo_fill(cr);

		// show min and max
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
		}
		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->info_font_size);

		sprintf(label,"Pre-Delay:");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,160-extents.width,286);
		cairo_show_text(cr,label);

		sprintf(label,"%.1fms",min_delay);
		cairo_move_to(cr,165,286);
		cairo_show_text(cr,label);


		sprintf(label,"Length:");
		cairo_text_extents (cr,label,&extents);
		cairo_move_to(cr,160-extents.width,295);
		cairo_show_text(cr,label);

		sprintf(label,"%.1fms",max_delay);
		cairo_move_to(cr,165,295);
		cairo_show_text(cr,label);

		//show impulse repsonse
		max_delay= ((gint)(max_delay/25 )+2) * 25;
		er=INV_DISPLAY_ERR(widget)->er;

		//show axis
		cairo_rectangle(cr, 5, 163, 1, 134 );
		cairo_fill(cr);

		cairo_rectangle(cr, 5, 230, 200, 1 );
		cairo_fill(cr);

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
		cairo_set_line_width(cr,1);

		for(i=0;i<er_size;i++) {
			x=5+200*(er->DelayActual/max_delay);
			y=2+fabs(er->GainL/max_gain)*65;
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.5);
			} else {
				cairo_set_source_rgba(cr, 0.2, 0.0, 1.0, 0.5);
			}
			cairo_rectangle(cr, x,230, 1, -y );
			cairo_fill(cr);

			y=2+fabs(er->GainR/max_gain)*65;

			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.4, 0.4, 0.4, 0.5);
			} else {
				cairo_set_source_rgba(cr, 0.0, 0.2, 1.0, 0.5);
			}
			cairo_rectangle(cr, x,230, 1, y );
			cairo_fill(cr);
			er++;
		}
	}

	/* source dest display */
	/* 310x280 centered at 350,155 */
	if(l!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_LENGTH] || 
	   w!=INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_WIDTH] ||
	   sLR!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_LR] ||
	   sFB!=INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_FB] ||
	   dLR!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_LR] ||
	   dFB!=INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_FB] ||
	   mode==INV_DISPLAY_ERR_DRAW_ALL) {

		sw=pow(w,0.5);
		sl=pow(l,0.5);
		scale=9999999;
		scale = 290/sw < scale ? 290/sw : scale;
		scale = 268/sl < scale ? 268/sl : scale;

		sw=sw*scale/2;
		sl=sl*scale/2;

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.1, 0.1, 0.1);
		} else {
			cairo_set_source_rgb(cr, 0.05, 0.05, 0.2);
		}
		cairo_rectangle(cr, 213, 24, 294, 272 );
		cairo_fill(cr);
	
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);
		if(active_dot==INV_DISPLAY_ERR_DOT_SOURCE) {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.2);
			} else {
				cairo_set_source_rgba(cr, 1.0, 1.0, 0.5, 0.2);
			}
			cairo_rectangle(cr, 360-(sw*0.99), 160-(sl*0.99), sw*1.98, sl*0.98 );
			cairo_fill_preserve(cr);
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.5);
			} else {
				cairo_set_source_rgba(cr, 1.0, 1.0, 0.5, 0.5);
			}
			cairo_stroke(cr);
		} else {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.05);
			} else {
				cairo_set_source_rgba(cr, 1.0, 1.0, 0.5, 0.05);
			}
			cairo_rectangle(cr, 360-(sw*0.99), 160-(sl*0.99), sw*1.98, sl*0.98 );
			cairo_fill(cr);
		}
	
		if(active_dot==INV_DISPLAY_ERR_DOT_DEST) {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.2);
			} else {
				cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.2);
			}
			cairo_rectangle(cr, 360-(sw*0.99), 160+(sl*0.01), sw*1.98, sl*0.98 );
			cairo_fill_preserve(cr);
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.5);
			} else {
				cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.5);
			}
			cairo_stroke(cr);
		} else {
			if(bypass==INV_PLUGIN_BYPASS) {
				cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.05);
			} else {
				cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.05);
			}
			cairo_rectangle(cr, 360-(sw*0.99), 160+(sl*0.01), sw*1.98, sl*0.98 );
			cairo_fill(cr);
		}

		cairo_set_antialias (cr,CAIRO_ANTIALIAS_NONE);
		cairo_set_line_width(cr,1);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
		} else {
			cairo_set_source_rgb(cr, 0.35, 0.35, 0.35);
		}

		cairo_rectangle(cr, 360-sw, 160-sl, sw*2, sl*2 );
		cairo_stroke(cr);
	
		cairo_set_antialias (cr,CAIRO_ANTIALIAS_DEFAULT);

		xc=360.0 + sLR*sw;
		yc=160+sl - sFB*sl*2;
		r=3;

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.8, 0.8, 0.0);
		}
		cairo_arc(cr,xc,yc,r,0,2*INV_PI);
		cairo_fill(cr);
		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.5, 0.5, 0.3);
		}
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->info_font_size);
		strcpy(label,"Source");
		cairo_text_extents (cr,label,&extents);
		if(l<w) {
			// horz
			if(sLR<0.0) {
				cairo_move_to(cr,xc+7,yc-extents.y_bearing/2);
			} else {
				cairo_move_to(cr,xc-7-extents.width,yc-extents.y_bearing/2);
			}
		} else {
			// vert
			if(sFB<0.75) {
				cairo_move_to(cr,xc-extents.width/2,yc-6);
			} else {
				cairo_move_to(cr,xc-extents.width/2,yc+5-extents.y_bearing);
			}
		}
		cairo_show_text(cr,label);

		xc=360.0 + dLR*sw;
		yc=160+sl - dFB*sl*2;
		r=3;

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 1.0, 0.1, 0.0);
		}		
		cairo_arc(cr,xc,yc,r,0,2*INV_PI);
		cairo_fill(cr);

		if(bypass==INV_PLUGIN_BYPASS) {
			cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
		} else {
			cairo_set_source_rgb(cr, 0.6, 0.35, 0.3);
		}		
		cairo_select_font_face(cr,"sans-serif",CAIRO_FONT_SLANT_NORMAL,CAIRO_FONT_WEIGHT_NORMAL);
		cairo_set_font_size(cr,INV_DISPLAY_ERR(widget)->info_font_size);
		strcpy(label,"Listener");
		cairo_text_extents (cr,label,&extents);
		if(l<w) {
			// horz
			if(dLR<0.0) {
				cairo_move_to(cr,xc+7,yc-1-extents.y_bearing/2);
			} else {
				cairo_move_to(cr,xc-7-extents.width,yc-1-extents.y_bearing/2);
			}
		} else {
			// vert
			if(dFB<0.25) {
				cairo_move_to(cr,xc-extents.width/2,yc-6);
			} else {
				cairo_move_to(cr,xc-extents.width/2,yc+5-extents.y_bearing);
			}
		}
		cairo_show_text(cr,label);
	}

	INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_LENGTH]=l;
	INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_WIDTH]=w;
	INV_DISPLAY_ERR(widget)->Lastroom[INV_DISPLAY_ERR_ROOM_HEIGHT]=h;
	INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_LR]=sLR;
	INV_DISPLAY_ERR(widget)->Lastsource[INV_DISPLAY_ERR_FB]=sFB;
	INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_LR]=dLR;
	INV_DISPLAY_ERR(widget)->Lastdest[INV_DISPLAY_ERR_FB]=dFB;
	INV_DISPLAY_ERR(widget)->Lastdiffusion=diffusion;

  	cairo_destroy(cr);
}


static void
inv_display_err_destroy(GtkObject *object)
{
	InvDisplayErrClass *klass;

	g_return_if_fail(object != NULL);
	g_return_if_fail(INV_IS_DISPLAY_ERR(object));

	klass = gtk_type_class(gtk_widget_get_type());

	if (GTK_OBJECT_CLASS(klass)->destroy) {
		(* GTK_OBJECT_CLASS(klass)->destroy) (object);
	}
}

static gboolean 
inv_display_err_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_DISPLAY_ERR(widget));

	INV_DISPLAY_ERR(widget)->active_dot=inv_display_err_find_active_dot(
			INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_LENGTH],
			INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_WIDTH],
			INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_LR],
			INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_FB],
			INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_LR],
			INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_FB],
			event->x,
			event->y);
	if(INV_DISPLAY_ERR(widget)->active_dot == INV_DISPLAY_ERR_DOT_SOURCE
	|| INV_DISPLAY_ERR(widget)->active_dot == INV_DISPLAY_ERR_DOT_DEST) {
		g_object_set(G_OBJECT(widget),"has-tooltip",FALSE,NULL);
		gtk_widget_set_state(widget,GTK_STATE_ACTIVE);
	    	gtk_widget_grab_focus(widget);
		inv_display_err_paint(widget,INV_DISPLAY_ERR_DRAW_DATA);
	}
	return TRUE;
}

static gboolean	
inv_display_err_motion_notify_event(GtkWidget *widget, GdkEventMotion *event)
{
	g_assert(INV_IS_DISPLAY_ERR(widget));
	switch(INV_DISPLAY_ERR(widget)->active_dot) {
		case INV_DISPLAY_ERR_DOT_SOURCE:
			inv_display_err_update_active_dot(INV_DISPLAY_ERR_DOT_SOURCE, 
							INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_LENGTH],
							INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_WIDTH],
							event->x,
							event->y,
							&(INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_LR]),
							&(INV_DISPLAY_ERR(widget)->source[INV_DISPLAY_ERR_FB]));
			inv_display_err_paint(widget,INV_DISPLAY_ERR_DRAW_DATA);
			return FALSE; 
			break;
		case INV_DISPLAY_ERR_DOT_DEST:
			inv_display_err_update_active_dot(INV_DISPLAY_ERR_DOT_DEST, 
							INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_LENGTH],
							INV_DISPLAY_ERR(widget)->room[INV_DISPLAY_ERR_ROOM_WIDTH],
							event->x,
							event->y,
							&(INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_LR]),
							&(INV_DISPLAY_ERR(widget)->dest[INV_DISPLAY_ERR_FB]));
			inv_display_err_paint(widget,INV_DISPLAY_ERR_DRAW_DATA);
			return FALSE; 
			break;
	}
	return TRUE; 

}

static gboolean 
inv_display_err_button_release_event (GtkWidget *widget, GdkEventButton *event)
{
	g_assert(INV_IS_DISPLAY_ERR(widget));

	if(INV_DISPLAY_ERR(widget)->active_dot == INV_DISPLAY_ERR_DOT_SOURCE
	|| INV_DISPLAY_ERR(widget)->active_dot == INV_DISPLAY_ERR_DOT_DEST) {
		INV_DISPLAY_ERR(widget)->active_dot= INV_DISPLAY_ERR_DOT_NONE;
		gtk_widget_set_state(widget,GTK_STATE_NORMAL);
		g_object_set(G_OBJECT(widget),"has-tooltip",TRUE,NULL);
		inv_display_err_paint(widget,INV_DISPLAY_ERR_DRAW_DATA);
	}
	return TRUE;
}


static void	
inv_display_err_screen(struct point3D *point, struct point2D *screen, struct point3D *camera, struct point3D *lookat)
{

	struct point3D 	CameraLookV;		// a vector describing where the camera is looking
	struct point3D 	ScreenXP;		// the x dimension of the plane the image is projected onto
	struct point3D 	ScreenYP;		// the y dimension of the plane the image is projected onto
	struct point3D 	CameraPointV;		// a vector describing where the point is in relation to the camera
	struct point3D 	ProjectedP;		// the location of the projected point on the x/y screen
	struct point3D 	ProjectedPCenter;	// the location of the projected point on the x/y screen relative to the lookat point
	float		CameraLookVLengthSqr,CameraLookVLength;
	float		ScreenXPLength;
	float		ScreenYPLength;
	float		d,t;

	//Establish the Vector Components of the Camera-to-Center Vector
	CameraLookV.x = (lookat->x - camera->x);
	CameraLookV.y = (lookat->y - camera->y);
	CameraLookV.z = (lookat->z - camera->z);
    
    	CameraLookVLengthSqr = CameraLookV.x*CameraLookV.x + CameraLookV.y*CameraLookV.y + CameraLookV.z*CameraLookV.z;
    	CameraLookVLength = pow(CameraLookVLengthSqr,0.5);
    
	// x plane of the projection
	ScreenXP.x = -CameraLookV.z;
	ScreenXP.y = 0;
	ScreenXP.z = CameraLookV.x;

    	ScreenXPLength = pow(ScreenXP.x*ScreenXP.x + ScreenXP.y*ScreenXP.y + ScreenXP.z*ScreenXP.z, 0.5);

	ScreenXP.x = ScreenXP.x / ScreenXPLength;
	ScreenXP.y = ScreenXP.y / ScreenXPLength;
	ScreenXP.z = ScreenXP.z / ScreenXPLength;

	// y plane of the projection
	ScreenYP.x = -(ScreenXP.z * CameraLookV.y);
	ScreenYP.y = (ScreenXP.z * CameraLookV.x) - (ScreenXP.x * CameraLookV.z);
	ScreenYP.z = (ScreenXP.x * CameraLookV.y);

	ScreenYPLength = pow(ScreenYP.x*ScreenYP.x + ScreenYP.y*ScreenYP.y + ScreenYP.z*ScreenYP.z, 0.5);

	ScreenYP.x = ScreenYP.x / ScreenYPLength;
	ScreenYP.y = ScreenYP.y / ScreenYPLength;
	ScreenYP.z = ScreenYP.z / ScreenYPLength;

	// scale by the camera distance
	ScreenXP.x = ScreenXP.x / CameraLookVLength;
	ScreenXP.y = ScreenXP.y / CameraLookVLength;
	ScreenXP.z = ScreenXP.z / CameraLookVLength;

	ScreenYP.x = ScreenYP.x / CameraLookVLength;
	ScreenYP.y = ScreenYP.y / CameraLookVLength;
	ScreenYP.z = ScreenYP.z / CameraLookVLength;

	// point -> camera vector
	CameraPointV.x = point->x - camera->x;
	CameraPointV.y = point->y - camera->y;
	CameraPointV.z = point->z - camera->z;

	// find where the vector intesects the projection plane
	d = (CameraPointV.x * CameraLookV.x) + (CameraPointV.y * CameraLookV.y) + (CameraPointV.z * CameraLookV.y);
	t = CameraLookVLengthSqr / d;

	ProjectedP.x = camera->x + CameraPointV.x*t;
	ProjectedP.y = camera->y + CameraPointV.y*t;
	ProjectedP.z = camera->z + CameraPointV.z*t ;

	//convert relative to lookat point
	ProjectedPCenter.x = (ProjectedP.x - lookat->x);
	ProjectedPCenter.y = (ProjectedP.y - lookat->y);
	ProjectedPCenter.z = (ProjectedP.z - lookat->z);

	// find screen coordinates
	screen->x = (ProjectedPCenter.x*ScreenXP.x) + (ProjectedPCenter.y*ScreenXP.y) + (ProjectedPCenter.z*ScreenXP.z);
	screen->y = (ProjectedPCenter.x*ScreenYP.x) + (ProjectedPCenter.y*ScreenYP.y) + (ProjectedPCenter.z*ScreenYP.z);

//	printf("point: x %f, y %f, z %f,   screen: x %f, y %f\n",point->x,point->y,point->z,screen->x,screen->y);


}

static gint
inv_display_err_find_active_dot(float l, float w, float sLR, float sFB, float dLR, float dFB, float x, float y)
{
	float sl,sw,scale,sx,sy,dx,dy,sdist,ddist;

	sw=pow(w,0.5);
	sl=pow(l,0.5);
	scale=9999999;
	scale = 290/sw < scale ? 290/sw : scale;
	scale = 268/sl < scale ? 268/sl : scale;

	sw=sw*scale/2;
	sl=sl*scale/2;

	sx=360.0 + sLR*sw;
	sy=160+sl - sFB*sl*2;

	dx=360.0 + dLR*sw;
	dy=160+sl - dFB*sl*2;

	sdist=pow(pow(sx-x,2)+pow(sy-y,2),0.5);
	ddist=pow(pow(dx-x,2)+pow(dy-y,2),0.5);

	if(sdist<5.0 && sdist<ddist) 
		return INV_DISPLAY_ERR_DOT_SOURCE;

	if(ddist<5.0 && ddist<sdist) 
		return INV_DISPLAY_ERR_DOT_DEST;

	return INV_DISPLAY_ERR_DOT_NONE;

}

static void 
inv_display_err_update_active_dot(gint dot, float l, float w, float x, float y, float *LR, float *FB)
{
	float sl,sw,scale;
	sw=pow(w,0.5);
	sl=pow(l,0.5);
	scale=9999999;
	scale = 290/sw < scale ? 290/sw : scale;
	scale = 268/sl < scale ? 268/sl : scale;

	sw=sw*scale/2;
	sl=sl*scale/2;

	*LR=(x-360.0)/sw;
	*FB=(1-(y-160)/sl)/2;

	if(*LR < -0.99) *LR = -0.99;
	if(*LR > 0.99) *LR = 0.99;

	switch(dot) {
		case INV_DISPLAY_ERR_DOT_SOURCE:
			if(*FB < 0.51) *FB = 0.51;
			if(*FB > 0.99) *FB = 0.99;
			break;
		case INV_DISPLAY_ERR_DOT_DEST:
			if(*FB < 0.01) *FB = 0.01;
			if(*FB > 0.49) *FB = 0.49;
			break;
	}
}


