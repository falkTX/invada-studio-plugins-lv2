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

#ifndef __DISPLAY_SPEC_H
#define __DISPLAY_SPEC_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_DISPLAY_SPEC_DRAW_ALL 0
#define INV_DISPLAY_SPEC_DRAW_DATA 1
#define INV_DISPLAY_SPEC_DRAW_ONE 2

#define INV_DISPLAY_SPEC(obj) GTK_CHECK_CAST(obj, inv_display_spec_get_type (), InvDisplaySpec)
#define INV_DISPLAY_SPEC_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_display_spec_get_type(), InvDisplaySpecClass)
#define INV_IS_DISPLAY_SPEC(obj) GTK_CHECK_TYPE(obj, inv_display_spec_get_type())


typedef struct _InvDisplaySpec InvDisplaySpec;
typedef struct _InvDisplaySpecClass InvDisplaySpecClass;


struct _InvDisplaySpec {
	GtkWidget widget;

	gint  bypass;
	float value[31];
	gint lastvalue[31];
	char label[31][6];

	struct colour mOff60,mOff12,mOff6,mOff0,overOff;
	struct colour mOn60, mOn12, mOn6, mOn0, overOn;  /* delta */

	gint font_size;

};

struct _InvDisplaySpecClass {
	GtkWidgetClass parent_class;
};


GtkType inv_display_spec_get_type(void);
GtkWidget * inv_display_spec_new();

void inv_display_spec_set_bypass(InvDisplaySpec *display_spec, gint num);
void inv_display_spec_set_value(InvDisplaySpec *display_spec, gint pos, float num);
void inv_display_spec_draw_now(InvDisplaySpec *display_spec, gint mode);

G_END_DECLS

#endif /* __DISPLAY_SPEC_H */

