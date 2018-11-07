SDCC  = sdcc -mmcs51 --std-sdcc99 -Iinclude $(CFLAGS)
SDAS  = sdas8051 -plo
SDAR ?= sdar rc

# core interrupts
DEFISRS  = \
	IE0 TF0 IE1 TF1 RI_TI_0 TF2 RESUME RI_TI_1 I2C IE5 IE6
# USB autovectored interrupts
DEFAUTOISRS += \
	SUDAV SOF SUTOK SUSPEND USBRESET HISPEED \
	EP0ACK EP0IN EP0OUT EP1IN EP1OUT EP2 EP4 EP6 EP8 \
	IBN EP0PING EP1PING EP2PING EP4PING EP6PING EP8PING \
	ERRLIMIT EP2ISOERR EP4ISOERR EP6ISOERR EP8ISOERR
# GPIF autovectored interrupts
DEFAUTOISRS += \
	EP2PF EP4PF EP6PF EP8PF \
	EP2EF EP4EF EP6EF EP8EF \
	EP2FF EP4FF EP6FF EP8FF GPIFDONE GPIFWF

MODELS = small medium large huge

OBJECTS_fx2 = xmemcpy.rel xmemclr.rel bswap.rel delay.rel syncdelay.rel i2c.rel eeprom.rel

OBJECTS_fx2isrs = autovec.rel \
	$(patsubst %,defisr_%.rel,$(DEFISRS)) \
	$(patsubst %,defautoisr_%.rel,$(DEFAUTOISRS))

OBJECTS_fx2usb = usb.rel \
	defusbdesc.rel \
	defusbsetup.rel \
	defusbsetconfig.rel defusbgetconfig.rel \
	defusbsetiface.rel defusbgetiface.rel \
	defusbhalt.rel \

OBJECTS_fx2usbmassstor = usbmassstor.rel

OBJECTS_fx2dfu = usbdfu.rel

OBJECTS_fx2uf2 = uf2scsi.rel uf2fat.rel

LIBRARIES = fx2 fx2isrs fx2usb fx2usbmassstor fx2dfu fx2uf2

all::
	@touch .stamp

define make-library
lib/$1/$2.lib: $(addprefix build/$1/,$(OBJECTS_$2))
	@mkdir -p $$(dir $$@)
	$(SDAR) $$@ $$^
endef

define make-model
all:: $(addprefix lib/$1/,$(addsuffix .lib,$(LIBRARIES)))

$(foreach library,$(LIBRARIES),$(eval $(call make-library,$1,$(library))))

build/$1/%.rel: %.c
	@mkdir -p $$(dir $$@)
	$(SDCC) --model-$1 -c -o $$@ $$<

build/$1/%.rel: %.asm
	@mkdir -p $$(dir $$@)
	$(SDAS) $$@ $$<

build/$1/defisr_%.rel: defisr.c
	$(SDCC) --model-$1 -c -o $$@ $$< -DISRNAME=isr_$$* -DINTNAME=_INT_$$*

build/$1/defautoisr_%.rel: defautoisr.c
	$(SDCC) --model-$1 -c -o $$@ $$< -DISRNAME=isr_$$*
endef

$(foreach model,$(MODELS),$(eval $(call make-model,$(model))))

clean:
	@rm -rf build/ lib/ .stamp *.lib

.PHONY: all clean

.SUFFIXES:
MAKEFLAGS += -r
