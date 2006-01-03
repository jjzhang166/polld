DESTDIR=
CFLAGS=-O2 -Wall
CONFIGFILE="\"/etc/polld\""
PIDFILE="\"/var/run/polld.pid\""
SLEEPTIME=10

all: polld

polld: polld.c
	gcc -o polld $(CFLAGS) -DCONFIGFILE=$(CONFIGFILE) -DSLEEPTIME=$(SLEEPTIME) -DPIDFULE=$(PIDFILE) polld.c

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
