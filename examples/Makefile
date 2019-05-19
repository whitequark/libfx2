SUBDIRS = blinky printf cdc-acm boot-uf2-dfu boot-dfu-spiflash

all:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} all; done

clean:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} clean; done
