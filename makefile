PREFIX ?= $(HOME)/.local
DATADIR ?= $(HOME)/.local/share/nvim2
CONFDIR ?= $(HOME)/.config/nvim2

# add cmd to create the folder for the db and the dd file

all: compile nvimi install db_install

compile: nvim2.cpp
	g++ -O2 nvim2.cpp -o nvim2

nvimi: nvimi.cpp
	g++ -O2 nvimi.cpp -o nvimi

install: nvim2 nvimi
	install -Dm755 nvim2 $(PREFIX)/bin/nvim2
	install -Dm755 nvimi $(PREFIX)/bin/nvimi

db_install:
	install -d $(DATADIR)
	touch $(DATADIR)/history_nvim.csv

config:
	install -d $(CONFDIR)
	touch $(CONFDIR)/nvim2.config

uninstall:
	rm -f $(PREFIX)/bin/nvim2
	rm -f $(PREFIX)/bin/nvimi

clear:
	rm nvim2

.PHONY: compile clear nvimi install uninstall db_install config


