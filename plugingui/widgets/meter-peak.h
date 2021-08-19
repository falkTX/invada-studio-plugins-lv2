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

#ifndef __METER_H
#define __METER_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_METER_DRAW_ALL 0
#define INV_METER_DRAW_L 1
#define INV_METER_DRAW_R 2

#define INV_METER_DRAW_MODE_TOZERO 0
#define INV_METER_DRAW_MODE_FROMZERO 1
#define INV_METER_DRAW_MODE_BIGTOZERO 2

#define INV_METER(obj) GTK_CHECK_CAST(obj, inv_meter_get_type (), InvMeter)
#define INV_METER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_meter_get_type(), InvMeterClass)
#define INV_IS_METER(obj) GTK_CHECK_TYPE(obj, inv_meter_get_type())


typedef struct _InvMeter InvMeter;
typedef struct _InvMeterClass InvMeterClass;


struct _InvMeter {
	GtkWidget widget;

	gint  bypass;
	gint  channels;
	gint  mode;

	float LdB;
	float RdB;

	gint lastLpos;
	gint lastRpos;

	struct colour mOff60,mOff12,mOff6,mOff0,overOff;
	struct colour mOn60, mOn12, mOn6, mOn0, overOn;  /* delta */

	gint label_font_size,scale_font_size;
};

struct _InvMeterClass {
	GtkWidgetClass parent_class;
};


GtkType inv_meter_get_type(void);
GtkWidget * inv_meter_new();

void inv_meter_set_bypass(InvMeter *meter, gint num);
void inv_meter_set_mode(InvMeter *meter, gint num);
void inv_meter_set_channels(InvMeter *meter, gint num);
void inv_meter_set_LdB(InvMeter *meter, float num);
void inv_meter_set_RdB(InvMeter *meter, float num);


G_END_DECLS

#endif /* __METER_H */

