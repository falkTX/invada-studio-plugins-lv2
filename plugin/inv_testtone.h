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

#define ITONE_URI		"http://invadarecords.com/plugins/lv2/testtone";
#define ITONE_GUI_URI		"http://invadarecords.com/plugins/lv2/testtone/gui";
#define ITONE_ACTIVE		0
#define ITONE_FREQ		1
#define ITONE_TRIM		2
#define ITONE_AUDIO_OUT		3
#define ITONE_METER_OUT 	4

/* control conversion function */
float convertParam(unsigned long param, float value, double sr);


