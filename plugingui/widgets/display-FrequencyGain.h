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

#ifndef __DISPLAYFG_H
#define __DISPLAYFG_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_DISPLAYFG_MODE_LPF 0
#define INV_DISPLAYFG_MODE_HPF 1

#define INV_DISPLAYFG_DRAW_ALL 0
#define INV_DISPLAYFG_DRAW_DATA 1

#define INV_DISPLAY_FG(obj) GTK_CHECK_CAST(obj, inv_display_fg_get_type (), InvDisplayFG)
#define INV_DISPLAY_FG_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_display_fg_get_type(), InvDisplayFGClass)
#define INV_IS_DISPLAY_FG(obj) GTK_CHECK_TYPE(obj, inv_display_fg_get_type())


typedef struct _InvDisplayFG InvDisplayFG;
typedef struct _InvDisplayFGClass InvDisplayFGClass;


struct _InvDisplayFG {
	GtkWidget widget;

	gint bypass;
	gint mode;
	float freq;
	float gain;

	float 		Lastfreq;
	float 		Lastgain;
	GtkStateType	Laststate;

	gint font_size;
};

struct _InvDisplayFGClass {
	GtkWidgetClass parent_class;
};


GtkType inv_display_fg_get_type(void);
GtkWidget * inv_display_fg_new();

void  inv_display_fg_set_bypass(InvDisplayFG *displayFG, gint num);
void  inv_display_fg_set_mode(InvDisplayFG *displayFG, gint num);
void  inv_display_fg_set_freq(InvDisplayFG *displayFG, float num);
void  inv_display_fg_set_gain(InvDisplayFG *displayFG, float num);
float inv_display_fg_get_freq(InvDisplayFG *displayFG);
float inv_display_fg_get_gain(InvDisplayFG *displayFG);


G_END_DECLS

#endif /* __DISPLAYFG_H */

