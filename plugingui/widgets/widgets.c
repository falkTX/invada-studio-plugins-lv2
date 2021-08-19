
/* 

    Common functions for invada widgets

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

#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "widgets.h"

gint
inv_choose_light_dark(GdkColor *bg,GdkColor *light,GdkColor *dark)
{

	float ld,dd;

	ld=pow(bg->red-light->red,2) + pow(bg->green-light->green,2) + pow(bg->blue-light->blue,2);
	dd=pow(bg->red-dark->red,2) + pow(bg->green-dark->green,2) + pow(bg->blue-dark->blue,2);

	return ld > dd ? 1 : 0;
}

gint	
inv_choose_font_size(cairo_t *cr, const char *family, cairo_font_slant_t slant, cairo_font_weight_t weight, double width, double height, const char *character)
{
	cairo_text_extents_t 	extents;
	gint 			i;

	cairo_select_font_face(cr,family,slant,weight);

	for(i=15;i>0;i--) {
		cairo_set_font_size(cr,i);
		cairo_text_extents (cr,character,&extents);
		//printf("Char: %s, font %i, width: %f, height: %f\n",character,i,extents.width,extents.height);
		if(extents.width <= width && extents.height <= height) {
			return i;
		}
	}

	return 0;

}
