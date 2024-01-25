# This file includes the fx2conf.mk configuration for fx2builds,
# and contains rules that should suffice for most firmware.
# It implements out-of-tree builds and dependency tracking for C,
# so that rebuilds work correctly after modifying headers.

# Configuration start

# See the configuration in fx2conf.mk as well.
include $(LIBFX2)/fx2conf.mk

# Name of our build product. Build will create $(FIRMWARE).ihex.
TARGET  	?= firmware

# List of C or assembly sources that comprise our firmware.
# The extension is determined automatically.
SOURCES 	?= main

# List of standard libraries to be included in the firmware.
LIBRARIES ?= fx2isrs

# Configuration end

OBJECTS 	 = \
	$(patsubst %,build/%.rel,$(SOURCES)) \
	$(patsubst %,$(LIBFX2)/lib/$(MODEL)/%.lib,$(LIBRARIES))

all: $(TARGET).ihex

$(TARGET).ihex: $(OBJECTS) $(LIBFX2)/.stamp
	$(SDCC) -o build/$@ $(OBJECTS)
	@cp build/$@ $@

$(LIBFX2)/.stamp: $(wildcard $(LIBFX2)/*.c $(LIBFX2)/*.asm $(LIBFX2)/include/*.h)
	$(MAKE) -C $(LIBFX2)

-include build/*.d
build/%.rel: %.c
	@mkdir -p $(dir $@)
	$(SDCC) -MQ $@ -MMD -o build/$*.d $<
	$(SDCC) -c -o $@ $<

build/%.rel: %.asm
	@mkdir -p $(dir $@)
	$(SDAS) $@ $<

clean:
	@rm -rf build/ $(TARGET).ihex

load: $(TARGET).ihex
	$(FX2LOAD) $<

.PHONY: all clean load

.SUFFIXES:
MAKEFLAGS += -r
