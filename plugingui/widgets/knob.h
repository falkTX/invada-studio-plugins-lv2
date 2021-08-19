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

#ifndef __KNOB_H
#define __KNOB_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_KNOB_DRAW_ALL     0
#define INV_KNOB_DRAW_DATA    1

#define INV_KNOB_SIZE_SMALL   50
#define INV_KNOB_SIZE_MEDIUM  64
#define INV_KNOB_SIZE_LARGE   80

#define INV_KNOB_CURVE_LINEAR 0
#define INV_KNOB_CURVE_LOG    1
#define INV_KNOB_CURVE_QUAD   2

#define INV_KNOB_MARKINGS_PAN 	0
#define INV_KNOB_MARKINGS_CUST10 1
#define INV_KNOB_MARKINGS_CUST12 2
#define INV_KNOB_MARKINGS_3   	3
#define INV_KNOB_MARKINGS_4   	4
#define INV_KNOB_MARKINGS_5   	5
#define INV_KNOB_MARKINGS_10  	10

#define INV_KNOB_HIGHLIGHT_L  -1
#define INV_KNOB_HIGHLIGHT_C  0
#define INV_KNOB_HIGHLIGHT_R  1

#define INV_KNOB(obj) GTK_CHECK_CAST(obj, inv_knob_get_type (), InvKnob)
#define INV_KNOB_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_knob_get_type(), InvKnobClass)
#define INV_IS_KNOB(obj) GTK_CHECK_TYPE(obj, inv_knob_get_type())


typedef struct _InvKnob InvKnob;
typedef struct _InvKnobClass InvKnobClass;


struct _InvKnob {
	GtkWidget widget;

	gint  bypass;

	gint  size;
	gint  curve;
	gint  markings;
	gint  highlight;
	gint  human;
	char  units[5];
	char  clow[10];
	char  cmid[10];
	char  chigh[10];
	float min;
	float max;
	float value;
	float lastvalue;
	float click_x;
	float click_y;

	GdkPixbuf *img_small;
	GdkPixbuf *img_med;
	GdkPixbuf *img_large;

	gint font_size;

};

struct _InvKnobClass {
	GtkWidgetClass parent_class;

};


GtkType inv_knob_get_type(void);
GtkWidget * inv_knob_new();

void inv_knob_set_bypass(InvKnob *knob, gint num);
void inv_knob_set_size(InvKnob *knob, gint num);
void inv_knob_set_curve(InvKnob *knob, gint num);
void inv_knob_set_markings(InvKnob *knob, gint num);
void inv_knob_set_custom(InvKnob *knob, gint pos, char *label);
void inv_knob_set_highlight(InvKnob *knob, gint num);
void inv_knob_set_human(InvKnob *knob);
void inv_knob_set_units(InvKnob *knob, char *units);
void inv_knob_set_min(InvKnob *knob, float num);
void inv_knob_set_max(InvKnob *knob, float num);
void inv_knob_set_value(InvKnob *knob, float num);
void inv_knob_set_tooltip(InvKnob *knob, gchar *tip);
float inv_knob_get_value(InvKnob *knob);


G_END_DECLS

#endif /* __KNOB_H */

