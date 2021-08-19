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

#define IFILTER_GUI_URI		"http://invadarecords.com/plugins/lv2/filter/gui"
#define IFILTER_MONO_LPF_URI	"http://invadarecords.com/plugins/lv2/filter/lpf/mono"
#define IFILTER_MONO_HPF_URI	"http://invadarecords.com/plugins/lv2/filter/hpf/mono"
#define IFILTER_STEREO_LPF_URI	"http://invadarecords.com/plugins/lv2/filter/lpf/stereo"
#define IFILTER_STEREO_HPF_URI	"http://invadarecords.com/plugins/lv2/filter/hpf/stereo"
#define IFILTER_BYPASS 		0
#define IFILTER_FREQ 		1
#define IFILTER_GAIN 		2
#define IFILTER_NOCLIP 		3
#define IFILTER_METER_INL  	4
#define IFILTER_METER_OUTL 	5
#define IFILTER_AUDIO_INL  	6
#define IFILTER_AUDIO_OUTL 	7
#define IFILTER_METER_DRIVE 	8
#define IFILTER_METER_INR  	9   /* not used in mono mode */
#define IFILTER_METER_OUTR 	10  /* not used in mono mode */
#define IFILTER_AUDIO_INR  	11  /* not used in mono mode */
#define IFILTER_AUDIO_OUTR 	12  /* not used in mono mode */



/* control conversion function */
float convertParam(unsigned long param, float value, double sr);

