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

#define IMETER_URI		"http://invadarecords.com/plugins/lv2/meter";
#define IMETER_GUI_URI		"http://invadarecords.com/plugins/lv2/meter/gui";
#define IMETER_BYPASS		0
#define IMETER_AUDIO_INL	1
#define IMETER_AUDIO_INR	2
#define IMETER_AUDIO_OUTL	3
#define IMETER_AUDIO_OUTR	4
#define IMETER_METER_L  	5
#define IMETER_METER_R  	6  
#define IMETER_VU_L  		7
#define IMETER_VU_R  		8 
#define IMETER_METER_PHASE 	9 
#define IMETER_SPEC_20 		10 
#define IMETER_SPEC_25 		11 
#define IMETER_SPEC_31 		12 
#define IMETER_SPEC_40 		13 
#define IMETER_SPEC_50 		14 
#define IMETER_SPEC_63 		15 
#define IMETER_SPEC_80 		16 
#define IMETER_SPEC_100 	17 
#define IMETER_SPEC_125 	18 
#define IMETER_SPEC_160 	19 
#define IMETER_SPEC_200 	20 
#define IMETER_SPEC_250 	21 
#define IMETER_SPEC_315 	22 
#define IMETER_SPEC_400 	23 
#define IMETER_SPEC_500 	24 
#define IMETER_SPEC_630 	25 
#define IMETER_SPEC_800 	26 
#define IMETER_SPEC_1000 	27 
#define IMETER_SPEC_1250 	28 
#define IMETER_SPEC_1600 	29 
#define IMETER_SPEC_2000 	30 
#define IMETER_SPEC_2500 	31 
#define IMETER_SPEC_3150 	32 
#define IMETER_SPEC_4000 	33 
#define IMETER_SPEC_5000 	34 
#define IMETER_SPEC_6300 	35 
#define IMETER_SPEC_8000 	36 
#define IMETER_SPEC_10000 	37 
#define IMETER_SPEC_12500 	38 
#define IMETER_SPEC_16000 	39 
#define IMETER_SPEC_20000 	40 


#define FILTER_COUNT		31


/* control conversion function */
float convertParam(unsigned long param, float value, double sr);


