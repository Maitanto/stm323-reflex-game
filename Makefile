PROJECT = program
BUILD_DIR = bin

SHARED_DIR = common
CFILES = main.c
CFILES += hardware.c
CFILES += sys/clock.c
CFILES += sys/init.c
CFILES += sys/power.c
CFILES += sys/serial_io.c
CFILES += sys/syscalls.c

DEVICE=stm32f446re
OPENCM3_TARGETS=stm32/f4
OOCD_FILE = board/st_nucleo_f4.cfg


VPATH += $(SHARED_DIR)
INCLUDES += $(patsubst %,-I%, . $(SHARED_DIR))
OPENCM3_DIR=libopencm3

CPPFLAGS= -Werror

all:

$(OPENCM3_DIR)/mk/genlink-config.mk \
$(OPENCM3_DIR)/mk/genlink-rules.mk:
	git submodule update --init $(OPENCM3_DIR)

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk

ifeq ($(wildcard main.c),)
main.c: main-orig.c
	cp $< $@
endif