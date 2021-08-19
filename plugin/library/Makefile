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
CFLAGS		+=	-I. -O3 -Wall -fomit-frame-pointer -fstrength-reduce -funroll-loops -ffast-math -c -fPIC -DPIC



all:			libinv_common.a

# RULES TO BUILD LIBRARIES FROM C CODE

libinv_common.a:	common.o	common.h 
			@echo "Creating libinv_common.a"
			@ar rcs libinv_common.a common.o


# OTHER TARGETS


targets:		libinv_common.a

always:	

clean:
			-rm -f *.a
			-rm -f *.o
			-rm -f *~

%.o: %.c
			@echo "Compiling $<"
			@$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@ 
