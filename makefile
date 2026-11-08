PREFIX ?= $(HOME)/.local
DATADIR ?= $(HOME)/.local/share/nvimpp
CONFDIR ?= $(HOME)/.config/nvimpp

# add cmd to create the folder for the db and the dd file

all: nvimpp nvimi install db_install config

nvimpp: nvimpp.cpp
	g++ -O2 nvimpp.cpp -o nvimpp

nvimi: nvimi.cpp
	g++ -O2 nvimi.cpp -o nvimi

install: nvimpp nvimi
	install -Dm755 nvimpp $(PREFIX)/bin/nvimpp
	install -Dm755 nvimi $(PREFIX)/bin/nvimi
	install -Dm644 nvimpp.1 $(PREFIX)/share/man/man1/nvimpp.1
	install -Dm644 nvimi.1 $(PREFIX)/share/man/man1/nvimi.1

db_install:
	install -d $(DATADIR)
	touch $(DATADIR)/history_nvim.csv

config:
	install -d $(CONFDIR)
	[ -f $(CONFDIR)/nvimpp.config ] || install -m644 nvimpp.config $(CONFDIR)/nvimpp.config

uninstall:
	rm -f $(PREFIX)/bin/nvimpp
	rm -f $(PREFIX)/bin/nvimi
	rm -f $(PREFIX)/share/man/man1/nvimpp.1
	rm -f $(PREFIX)/share/man/man1/nvimi.1

clear:
	rm -f nvimpp nvimi

.PHONY: all clear install uninstall db_install config


