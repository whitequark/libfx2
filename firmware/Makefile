SUBDIRS = library blinky boot-cypress

all:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} all; done

clean:
	@set -e; for dir in $(SUBDIRS); do $(MAKE) -C $${dir} clean; done
