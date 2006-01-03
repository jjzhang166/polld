DESTDIR=
CFLAGS=-O2 -Wall
CC=gcc

CONFIGFILE=/etc/polld
PIDFILE=/var/run/polld.pid
SLEEPTIME=10
VERSION=0.2

.PHONY: all clean install dist

all: polld

polld: polld.c
	$(CC) -o polld $(CFLAGS) -DCONFIGFILE=\"$(CONFIGFILE)\" -DSLEEPTIME=$(SLEEPTIME) -DPIDFILE=\"$(PIDFILE)\" -DVERSION=\"$(VERSION)\" polld.c

clean:
	rm -f polld

install: all
	install -m 755 -d $(DESTDIR)/etc
	install -m 755 -d $(DESTDIR)/usr/bin
	install -m 755 -d $(DESTDIR)/usr/share/man/man1
	install -m 755 -d $(DESTDIR)/usr/share/man/man5
	install -m 644 config $(DESTDIR)/etc/polld
	install -m 755 polld $(DESTDIR)/usr/bin/
	install -m 644 polld.1 $(DESTDIR)/usr/share/man/man1
	install -m 644 polld.5 $(DESTDIR)/usr/share/man/man5

dist: clean
	# we don't know whether dh_clean is installed, but do this cleanup
	-dh_clean
	mkdir -p polld-$(VERSION)/debian
	cp polld.* config COPYING Makefile README polld-$(VERSION)/
	cp debian/* polld-$(VERSION)/debian/
	tar cfj ../polld-$(VERSION).tar.bz2 polld-$(VERSION)
	rm -rf polld-$(VERSION)
	
