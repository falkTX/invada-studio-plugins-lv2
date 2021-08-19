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

#define ICOMP_GUI_URI		"http://invadarecords.com/plugins/lv2/compressor/gui"
#define ICOMP_MONO_URI		"http://invadarecords.com/plugins/lv2/compressor/mono"
#define ICOMP_STEREO_URI	"http://invadarecords.com/plugins/lv2/compressor/stereo"
#define ICOMP_BYPASS 		0
#define ICOMP_RMS 		1
#define ICOMP_ATTACK 		2
#define ICOMP_RELEASE 		3
#define ICOMP_THRESH 		4
#define ICOMP_RATIO 		5
#define ICOMP_GAIN 		6
#define ICOMP_NOCLIP 		7
#define ICOMP_METER_GR 		8
#define ICOMP_METER_DRIVE	9
#define ICOMP_METER_INL		10
#define ICOMP_METER_OUTL	11
#define ICOMP_AUDIO_INPUTL  	12
#define ICOMP_AUDIO_OUTPUTL 	13
#define ICOMP_METER_INR		14  /* not used in mono mode */
#define ICOMP_METER_OUTR	15  /* not used in mono mode */
#define ICOMP_AUDIO_INPUTR  	16  /* not used in mono mode */
#define ICOMP_AUDIO_OUTPUTR 	17  /* not used in mono mode */

float convertParam(unsigned long param, float value, double sr);



