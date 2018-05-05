# This file contains only configuration for fx2 builds, no rules.
# Use it if you need tight control over the build system.

# -- Configuration start --

VID       ?= 04B4
PID       ?= 8613

MODEL     ?= small
CODE_SIZE ?= 0x3e00
XRAM_SIZE ?= 0x0200
CFLAGS    ?=

# -- Configuration end --

# Ensure we can access fx2ng without installation.
PYTHONPATH = $(LIBFX2)/../../software
export PYTHONPATH

SDCCFLAGS  = \
	--iram-size 0x100 \
	--code-size $(CODE_SIZE) \
	--xram-loc  $(CODE_SIZE) \
	--xram-size $(XRAM_SIZE) \
	--std-sdcc99 \
	--model-$(MODEL) \
	$(CFLAGS) \
	-I$(LIBFX2)/include \
	-L$(LIBFX2)/lib/$(MODEL)
ifeq ($(V),1)
SDCCFLAGS += -V
endif

SDCC       = sdcc -mmcs51 $(SDCCFLAGS)
SDAS       = sdas8051 -plo
FX2LOAD    = python3 -m fx2.fx2tool -d $(VID):$(PID) load
