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

#ifndef __DISPLAY_COMP_H
#define __DISPLAY_COMP_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"

G_BEGIN_DECLS

#define INV_DISPLAYCOMP_DRAW_ALL 0
#define INV_DISPLAYCOMP_DRAW_DATA 1

#define INV_DISPLAY_COMP(obj) GTK_CHECK_CAST(obj, inv_display_comp_get_type (), InvDisplayComp)
#define INV_DISPLAY_COMP_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_display_comp_get_type(), InvDisplayCompClass)
#define INV_IS_DISPLAY_COMP(obj) GTK_CHECK_TYPE(obj, inv_display_comp_get_type())


typedef struct _InvDisplayComp InvDisplayComp;
typedef struct _InvDisplayCompClass InvDisplayCompClass;


struct _InvDisplayComp {
	GtkWidget widget;

	gint bypass;
	float rms;
	float attack;
	float release;
	float threshold;
	float ratio;
	float gain;

	float Lastrms;
	float Lastattack;
	float Lastrelease;
	float Lastthreshold;
	float Lastratio;
	float Lastgain;

	float SIG[292], SIGmax;
	float RMS[292];
	float ENV[292];

	float header_font_size,label_font_size,info_font_size;
};

struct _InvDisplayCompClass {
	GtkWidgetClass parent_class;
};


GtkType inv_display_comp_get_type(void);
GtkWidget * inv_display_comp_new();

void inv_display_comp_set_bypass(InvDisplayComp *displayComp, gint num);
void inv_display_comp_set_rms(InvDisplayComp *displayComp, float num);
void inv_display_comp_set_attack(InvDisplayComp *displayComp, float num);
void inv_display_comp_set_release(InvDisplayComp *displayComp, float num);
void inv_display_comp_set_threshold(InvDisplayComp *displayComp, float num);
void inv_display_comp_set_ratio(InvDisplayComp *displayComp, float num);
void inv_display_comp_set_gain(InvDisplayComp *displayComp, float num);


G_END_DECLS

#endif /* __DISPLAY_COMP_H */

