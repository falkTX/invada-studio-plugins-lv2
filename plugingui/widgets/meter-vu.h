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

#ifndef __VU_METER_H
#define __VU_METER_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_VU_METER_DRAW_ALL 0
#define INV_VU_METER_DRAW_DATA 1

#define INV_VU_METER(obj) GTK_CHECK_CAST(obj, inv_vu_meter_get_type (), InvVuMeter)
#define INV_VU_METER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_vu_meter_get_type(), InvVuMeterClass)
#define INV_IS_VU_METER(obj) GTK_CHECK_TYPE(obj, inv_vu_meter_get_type())


typedef struct _InvVuMeter InvVuMeter;
typedef struct _InvVuMeterClass InvVuMeterClass;


struct _InvVuMeter {
	GtkWidget widget;

	gint  bypass;
	float value;
	float lastvalue;

	gint  headroom;
	float scale;

	float cx,cy;
	float r[4],a[5];

	struct point2D  dbm20[3];
	struct point2D  dbm10[3];
	struct point2D  dbm07[3];
	struct point2D  dbm05[3];
	struct point2D  dbm03[3];
	struct point2D  dbm02[3];
	struct point2D  dbm01[3];
	struct point2D  db00[3];
	struct point2D  dbp01[3];
	struct point2D  dbp02[3];
	struct point2D  dbp03[3];

	struct point2D  cp[2];

	gint label_font_size,scale_font_size;
};

struct _InvVuMeterClass {
	GtkWidgetClass parent_class;
};


GtkType inv_vu_meter_get_type(void);
GtkWidget * inv_vu_meter_new();

void inv_vu_meter_set_bypass(InvVuMeter *meter, gint num);
void inv_vu_meter_set_value(InvVuMeter *meter, float num);
void inv_vu_meter_set_headroom(InvVuMeter *meter, gint num);


G_END_DECLS

#endif /* __VU_METER_VU_H */

