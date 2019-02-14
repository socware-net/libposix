PREFIX ?= $(shell pwd)/../prefix/$(CROSS:%-=%)
SOC?=sim
NAME   :=posix
TARGET :=arm-none-eabi
CROSS  :=$(TARGET)-
CPU    :=arm

INCLUDE:=-Iinclude -I$(PREFIX)/include -Isrc
COPTS  ?=-march=armv7-m -mthumb -fsingle-precision-constant -Wdouble-promotion -mfpu=fpv4-sp-d16 -mfloat-abi=hard
AARCH  :=7
MOPTS  :=$(COPTS) \
	-DCFG_AARCH=$(AARCH) \
	-fno-builtin -fno-common \
	-ffunction-sections -fdata-sections -fshort-enums
CONFIG :=
ASFLAGS:=$(MOPTS) $(CONFIG) -O2 -g -Wall -Werror -D __ASSEMBLY__
CFLAGS :=$(MOPTS) $(CONFIG) -O2 -g -Wall -Werror
LSCRIPT?=rom.ld
LDFLAGS:=$(MOPTS) -g -nostartfiles -nodefaultlibs -L . -L$(PREFIX)/lib -Tbin/$(SOC)/$(LSCRIPT)
LDFLAGS+= -Wl,--start-group -lhcos -lc -lgcc $(SOC_LIB) -Wl,--end-group -Wl,--gc-sections

MSCRIPT:=$(PREFIX)/share/mod.ld
LIB    :=lib$(NAME).a

ALL    :=$(LIB)
CLEAN  :=
CPU    :=arm

VPATH  :=src
VOBJ   := \
	$(patsubst %.S,%.o, \
		$(patsubst %.c,%.o, \
		$(patsubst %.cpp, %.o, \
			$(notdir $(foreach DIR,src src/$(SOC),\
				$(wildcard $(DIR)/*.S)	\
				$(wildcard $(DIR)/*.c) 	\
				$(wildcard $(DIR)/*.cpp))))))
default:all

include $(PREFIX)/share/Makefile.rule

.PHONY:$(PREFIX)/bin/$(SOC)
$(PREFIX)/bin/$(SOC):
	rm -rf $@
	cp -r bin/$(SOC) $@

F?=hello.elf

ddd:openocd.gdb $(F)
	ddd --debugger $(CROSS)gdb -x openocd.gdb $(F)
	
ddd-attach:
	echo "target remote 127.0.0.1:3333" > attach.gdb
	ddd --debugger $(CROSS)gdb -x attach.gdb $(F)

