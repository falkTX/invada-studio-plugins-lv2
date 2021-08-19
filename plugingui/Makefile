#   Copyright (C) 2009 Fraser Stuart
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#   


# GENERAL

CC		?=	gcc
CFLAGS		+=	-I. `pkg-config --cflags cairo gtk+-2.0 lv2` -O3 -Wall -fomit-frame-pointer -funroll-loops -ffast-math -c -fPIC -DPIC
CFLAGS		+=	-Wno-deprecated-declarations
LDFLAGS		+=	-shared -Lwidgets -L../plugin/library -linv_widgets -linv_common -lm `pkg-config --libs cairo gtk+-2.0 lv2`


GUIS		=	inv_compressor_gui.so \
			inv_delay_gui.so \
			inv_erreverb_gui.so \
			inv_filter_gui.so \
			inv_input_gui.so \
			inv_meter_gui.so \
			inv_phaser_gui.so \
			inv_testtone_gui.so \
			inv_tube_gui.so \


all:	$(GUIS) 
     
# RULES TO BUILD GUIS FROM C CODE

inv_compressor_gui.so: inv_compressor_gui.o	../plugin/inv_compressor.h	inv_compressor_gui.h   
inv_delay_gui.so:      inv_delay_gui.o		../plugin/inv_delay.h		inv_delay_gui.h   
inv_erreverb_gui.so:   inv_erreverb_gui.o	../plugin/inv_erreverb.h	inv_erreverb_gui.h   
inv_filter_gui.so:     inv_filter_gui.o		../plugin/inv_filter.h		inv_filter_gui.h   
inv_input_gui.so:      inv_input_gui.o		../plugin/inv_input.h		inv_input_gui.h   
inv_meter_gui.so:      inv_meter_gui.o		../plugin/inv_meter.h		inv_meter_gui.h  
inv_phaser_gui.so:     inv_phaser_gui.o		../plugin/inv_phaser.h		inv_phaser_gui.h  
inv_testtone_gui.so:   inv_testtone_gui.o	../plugin/inv_testtone.h	inv_testtone_gui.h  
inv_tube_gui.so:       inv_tube_gui.o		../plugin/inv_tube.h		inv_tube_gui.h 


# OTHER TARGETS

targets: $(GUIS)

always:	

clean:
	-rm -f *.so
	-rm -f *.o
	-rm -f *~

%.o: %.c
	@echo "Compiling $<"
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@ 

%.so: %.o
	@echo "Creating  $@"
	@$(CC) -o $@ $< $(LDFLAGS)

