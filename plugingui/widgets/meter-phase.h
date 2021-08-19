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

#ifndef __PHASE_METER_H
#define __PHASE_METER_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_PHASE_METER_DRAW_ALL 0
#define INV_PHASE_METER_DRAW_DATA 1

#define INV_PHASE_METER(obj) GTK_CHECK_CAST(obj, inv_phase_meter_get_type (), InvPhaseMeter)
#define INV_PHASE_METER_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_phase_meter_get_type(), InvPhaseMeterClass)
#define INV_IS_PHASE_METER(obj) GTK_CHECK_TYPE(obj, inv_phase_meter_get_type())


typedef struct _InvPhaseMeter InvPhaseMeter;
typedef struct _InvPhaseMeterClass InvPhaseMeterClass;


struct _InvPhaseMeter {
	GtkWidget widget;

	gint  bypass;
	float phase;

	struct colour mOff0,mOff30,mOff45,mOff60,mOff90;
	struct colour mOn0, mOn30, mOn45, mOn60, mOn90;  /* delta */

	gint font_size;
};

struct _InvPhaseMeterClass {
	GtkWidgetClass parent_class;
};


GtkType inv_phase_meter_get_type(void);
GtkWidget * inv_phase_meter_new();

void inv_phase_meter_set_bypass(InvPhaseMeter *meter, gint num);
void inv_phase_meter_set_phase(InvPhaseMeter *meter, float num);


G_END_DECLS

#endif /* __PHASE_METER_PHASE_H */

