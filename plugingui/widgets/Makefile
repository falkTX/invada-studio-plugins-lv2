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

CC		=	gcc
LD		=	ld
CFLAGS		=	-I. `pkg-config --cflags gtk+-2.0`  -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC



all:			libinv_widgets.a

# RULES TO BUILD LIBRARIES FROM C CODE

libinv_widgets.a:	widgets.o		widgets.h \
			display-Compressor.o    display-Compressor.h \
			display-FrequencyGain.o display-FrequencyGain.h \
			display-ErReverb.o      display-ErReverb.h \
			display-Spectrograph.o  display-Spectrograph.h \
			knob.o                  knob.h \
			lamp.o                  lamp.h \
			meter-peak.o            meter-peak.h \
			meter-phase.o           meter-phase.h \
			meter-vu.o              meter-vu.h \
			switch-toggle.o         switch-toggle.h 
			@echo "Creating libinv_widgets.a"
			@ar rcs libinv_widgets.a \
				widgets.o \
				display-Compressor.o \
				display-ErReverb.o \
				display-FrequencyGain.o \
				display-Spectrograph.o \
				knob.o \
				lamp.o \
				meter-peak.o \
				meter-phase.o \
				meter-vu.o \
				switch-toggle.o
				

# OTHER TARGETS


targets:		libinv_widgets.a

always:	

clean:
			-rm -f *.a
			-rm -f *.o
			-rm -f *~

%.o: %.c
			@echo "Compiling $<"
			@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@ 


