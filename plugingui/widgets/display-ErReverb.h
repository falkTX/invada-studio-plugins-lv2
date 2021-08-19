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

#ifndef __DISPLAY_ERR_H
#define __DISPLAY_ERR_H

#include <gtk/gtk.h>
#include <cairo.h>
#include "widgets.h"
#include "../../plugin/library/common.h"

G_BEGIN_DECLS

#define INV_DISPLAY_ERR_DRAW_ALL 0
#define INV_DISPLAY_ERR_DRAW_DATA 1

#define INV_DISPLAY_ERR_ROOM_LENGTH 0
#define INV_DISPLAY_ERR_ROOM_WIDTH  1
#define INV_DISPLAY_ERR_ROOM_HEIGHT 2

#define INV_DISPLAY_ERR_LR 0
#define INV_DISPLAY_ERR_FB 1

#define INV_DISPLAY_ERR_DOT_NONE 0
#define INV_DISPLAY_ERR_DOT_SOURCE 1
#define INV_DISPLAY_ERR_DOT_DEST 2

#define INV_DISPLAY_ERR(obj) GTK_CHECK_CAST(obj, inv_display_err_get_type (), InvDisplayErr)
#define INV_DISPLAY_ERR_CLASS(klass) GTK_CHECK_CLASS_CAST(klass, inv_display_err_get_type(), InvDisplayErrClass)
#define INV_IS_DISPLAY_ERR(obj) GTK_CHECK_TYPE(obj, inv_display_err_get_type())


typedef struct _InvDisplayErr InvDisplayErr;
typedef struct _InvDisplayErrClass InvDisplayErrClass;


struct _InvDisplayErr {
	GtkWidget widget;
	
	gint active_dot;
	gint bypass;

	float room[3];
	float source[2];
	float dest[2];
	float diffusion;

	float Lastroom[3];
	float Lastsource[2];
	float Lastdest[2];
	float Lastdiffusion;

	struct ERunit * er;
	gint er_size;

	float header_font_size,info_font_size;
};

struct _InvDisplayErrClass {
	GtkWidgetClass parent_class;
};


GtkType inv_display_err_get_type(void);
GtkWidget * inv_display_err_new();

void inv_display_err_set_bypass(InvDisplayErr *displayErr, gint num);
gint inv_display_err_get_active_dot(InvDisplayErr *displayErr);
float inv_display_err_get_source(InvDisplayErr *displayErr, gint axis);
float inv_display_err_get_dest(InvDisplayErr *displayErr, gint axis);
void inv_display_err_set_room(InvDisplayErr *displayErr, gint axis, float num);
void inv_display_err_set_source(InvDisplayErr *displayErr, gint axis, float num);
void inv_display_err_set_dest(InvDisplayErr *displayErr, gint axis, float num);
void inv_display_err_set_diffusion(InvDisplayErr *displayErr, float num);

G_END_DECLS

#endif /* __DISPLAY_ERR_H */

