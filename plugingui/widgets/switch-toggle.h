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

#ifndef __SWITCH_TOGGLE_H
#define __SWITCH_TOGGLE_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_SWITCH_TOGGLE_DRAW_ALL 0
#define INV_SWITCH_TOGGLE_DRAW_DATA  1

#define INV_SWITCH_TOGGLE_OFF 0
#define INV_SWITCH_TOGGLE_ON  1

#define INV_SWITCH_TOGGLE(obj) GTK_CHECK_CAST(obj, inv_switch_toggle_get_type (), InvSwitchToggle)
#define INV_SWITCH_TOGGLE_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_switch_toggle_get_type(), InvSwitchToggleClass)
#define INV_IS_SWITCH_TOGGLE(obj) GTK_CHECK_TYPE(obj, inv_switch_toggle_get_type())


typedef struct _InvSwitchToggle InvSwitchToggle;
typedef struct _InvSwitchToggleClass InvSwitchToggleClass;


struct _InvSwitchToggle {
	GtkWidget widget;

	gint bypass;
	gint state;
	gint laststate;
	float value;

	float on_value;
	float off_value;

	struct colour on;
	struct colour off;

	char on_text[15];
	char off_text[15];

	char label[15];

	GdkPixbuf *img_on;
	GdkPixbuf *img_off;

	gint	font_size;

};

struct _InvSwitchToggleClass {
	GtkWidgetClass parent_class;
};


GtkType inv_switch_toggle_get_type(void);
GtkWidget * inv_switch_toggle_new();

void inv_switch_toggle_set_bypass(InvSwitchToggle *switch_toggle, gint num);
void inv_switch_toggle_toggle(InvSwitchToggle *switch_toggle);
void inv_switch_toggle_set_state(InvSwitchToggle *switch_toggle, gint state);
void inv_switch_toggle_set_value(InvSwitchToggle *switch_toggle, gint state, float value);
void inv_switch_toggle_set_colour(InvSwitchToggle *switch_toggle, gint state, float R, float G, float B);
void inv_switch_toggle_set_text(InvSwitchToggle *switch_toggle, gint state, const char *text);
void inv_switch_toggle_set_label(InvSwitchToggle *switch_toggle, const char *text);
void inv_switch_toggle_set_tooltip(InvSwitchToggle *switch_toggle, gchar *tip);
float inv_switch_toggle_get_value(InvSwitchToggle *switch_toggle);

G_END_DECLS

#endif /* __SWITCH_TOGGLE_H */

