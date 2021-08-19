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

#ifndef __INV_WIDGETS_H
#define __INV_WIDGETS_H

#include <gtk/gtk.h>
#include <cairo.h>

#define INV_PLUGIN_ACTIVE 0
#define INV_PLUGIN_BYPASS 1

#define INV_PI 3.1415926535

struct point2D {
	float x; 
	float y;
};

struct point3D {
	float x; 
	float y;
	float z;
};

struct colour {
	float R; 
	float G;
	float B;
};

gint	inv_choose_light_dark(GdkColor *bg,GdkColor *light,GdkColor *dark);
gint	inv_choose_font_size(cairo_t *cr, const char *family, cairo_font_slant_t slant, cairo_font_weight_t weight, double width, double height, const char *character);


#endif /* __INV_WIDGETS_H */
