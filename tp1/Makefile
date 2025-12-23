#!/usr/bin/make -f

include ../utils/config.mk

# Pour que <configuration_files/...> soit trouvable depuis tp_exam/
CFLAGS   += -I. -I./configuration_files
CPPFLAGS += -I. -I./configuration_files

# tp.c + tous les .c de configuration_files/**
tp_exam_srcs := tp.c $(shell find configuration_files -name '*.c' -type f 2>/dev/null)
objects      += $(tp_exam_srcs:.c=.o)

include ../utils/rules.mk
#-include $(dependencies)