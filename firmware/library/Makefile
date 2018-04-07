SDCC = sdcc -mmcs51 -Iinclude
SDAS = sdas8051 -plo
SDAR = sdar rc

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

OBJECTS_fx2 = xmemcpy.rel delay.rel i2c.rel eeprom.rel

OBJECTS_fx2isrs = autovec.rel \
	$(patsubst %,defisr_%.rel,$(DEFISRS)) \
	$(patsubst %,defautoisr_%.rel,$(DEFAUTOISRS))

OBJECTS_fx2usb = usb.rel \
	defusbdesc.rel defusbconfig.rel defusbiface.rel defusbhalt.rel defusbsetup.rel

all: fx2.lib fx2isrs.lib fx2usb.lib
	@touch .stamp

fx2.lib: $(addprefix build/,$(OBJECTS_fx2))
	$(SDAR) $@ $^

fx2isrs.lib: $(addprefix build/,$(OBJECTS_fx2isrs))
	$(SDAR) $@ $^

fx2usb.lib: $(addprefix build/,$(OBJECTS_fx2usb))
	$(SDAR) $@ $^

build/%.rel: %.c
	@mkdir -p $(dir $@)
	$(SDCC) -c -o $@ $<

build/%.rel: %.asm
	@mkdir -p $(dir $@)
	$(SDAS) $@ $<

build/defisr_%.rel: defisr.c
	$(SDCC) -c -o $@ $< -DISRNAME=isr_$* -DINTNAME=_INT_$*

build/defautoisr_%.rel: defautoisr.c
	$(SDCC) -c -o $@ $< -DISRNAME=isr_$*

clean:
	@rm -f build/* *.lib

.PHONY: all clean
