DESTDIR=
CFLAGS=-O2 -Wall

all: polld

polld: polld.c
	gcc -o polld $(CFLAGS) polld.c

clean:
	rm -f polld

install: all
	install -m 755 -d $(DESTDIR)/etc
	install -m 755 -d $(DESTDIR)/usr/bin
	install -m 755 -d $(DESTDIR)/usr/share/man/man1
	install -m 644 config $(DESTDIR)/etc/polld
	install -m 755 polld $(DESTDIR)/usr/bin/
	install -m 644 polld.1 $(DESTDIR)/usr/share/man/man1
