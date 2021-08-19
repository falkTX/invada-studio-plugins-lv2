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


#define IPHASER_GUI_URI		"http://invadarecords.com/plugins/lv2/phaser/gui"
#define IPHASER_MONO_URI	"http://invadarecords.com/plugins/lv2/phaser/mono"
#define IPHASER_STEREO_URI	"http://invadarecords.com/plugins/lv2/phaser/stereo"
#define IPHASER_SUM_URI		"http://invadarecords.com/plugins/lv2/phaser/sum"
#define IPHASER_BYPASS		0
#define IPHASER_CYCLE		1
#define IPHASER_PHASE 		2
#define IPHASER_WIDTH 		3
#define IPHASER_DEPTH 		4
#define IPHASER_NOCLIP 		5
#define IPHASER_LAMP_NOCLIP	6
#define IPHASER_LAMP_L		7
#define IPHASER_LAMP_R 		8
#define IPHASER_AUDIO_OUTL 	9
#define IPHASER_AUDIO_OUTR 	10 
#define IPHASER_METER_OUTL 	11
#define IPHASER_METER_OUTR 	12
#define IPHASER_AUDIO_INL  	13
#define IPHASER_METER_INL   	14
#define IPHASER_AUDIO_INR  	15   /* not used in mono in mode */ 
#define IPHASER_METER_INR  	16   /* not used in mono in mode */ 

#define PHASER_SPACE_SIZE 45   /* size in milli seconds */

/* control conversion function */
float convertParam(unsigned long param, float value, double sr);

