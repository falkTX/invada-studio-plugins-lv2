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


#define IERR_GUI_URI	"http://invadarecords.com/plugins/lv2/erreverb/gui"
#define IERR_MONO_URI	"http://invadarecords.com/plugins/lv2/erreverb/mono"
#define IERR_SUM_URI	"http://invadarecords.com/plugins/lv2/erreverb/sum"
#define IERR_BYPASS	0
#define IERR_ROOMLENGTH	1
#define IERR_ROOMWIDTH 	2
#define IERR_ROOMHEIGHT	3
#define IERR_SOURCELR 	4
#define IERR_SOURCEFB 	5
#define IERR_DESTLR 	6
#define IERR_DESTFB 	7
#define IERR_HPF 	8
#define IERR_WARMTH 	9
#define IERR_DIFFUSION 	10
#define IERR_AUDIO_OUTL 11
#define IERR_AUDIO_OUTR 12 
#define IERR_AUDIO_INL  13
#define IERR_METER_IN   14
#define IERR_METER_OUTL 15
#define IERR_METER_OUTR 16
#define IERR_AUDIO_INR  17   /* not used in mono in mode */ 

#define REVERB_SPACE_SIZE 2   /* size in seconds */
#define OBJECT_HEIGHT     1.5 /* the height of the sound source and the listener */



/* works out the reflection details */
void calculateIReverbERWrapper(LV2_Handle instance);

/* control conversion function */
float convertParam(unsigned long param, float value, double sr);

