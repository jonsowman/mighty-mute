##
## This file is part of the libopencm3 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This library is free software: you can redistribute it and/or modify
## it under the terms of the GNU Lesser General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This library is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU Lesser General Public License for more details.
##
## You should have received a copy of the GNU Lesser General Public License
## along with this library.  If not, see <http://www.gnu.org/licenses/>.
##

BINARY = mightymute
#DEFS += -DSTM32F4
#BUILD_DIR = bin
#SHARED_DIR = .

CFILES += main.c

DEVICE = stm32f401ret6

# You shouldn't have to edit anything below here.
#VPATH += $(SHARED_DIR)
#INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
#OPENCM3_DIR=libopencm3

#include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
#include $(OPENCM3_DIR)/mk/genlink-rules.mk
