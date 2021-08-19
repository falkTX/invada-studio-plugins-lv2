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

#define IINPUT_URI		"http://invadarecords.com/plugins/lv2/input";
#define IINPUT_GUI_URI		"http://invadarecords.com/plugins/lv2/input/gui";
#define IINPUT_BYPASS		0
#define IINPUT_PHASEL		1
#define IINPUT_PHASER		2
#define IINPUT_GAIN		3
#define IINPUT_PAN		4
#define IINPUT_WIDTH		5
#define IINPUT_NOCLIP		6
#define IINPUT_AUDIO_INL	7
#define IINPUT_AUDIO_INR	8
#define IINPUT_AUDIO_OUTL	9
#define IINPUT_AUDIO_OUTR	10
#define IINPUT_METER_INL  	11
#define IINPUT_METER_INR  	12  
#define IINPUT_METER_OUTL 	13
#define IINPUT_METER_OUTR 	14 
#define IINPUT_METER_PHASE 	15 
#define IINPUT_METER_DRIVE 	16 

/* control conversion function */
float convertParam(unsigned long param, float value, double sr);


