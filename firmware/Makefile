SUBDIRS = library bootloader blinky

all:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} all; done

clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $${dir} clean; done
