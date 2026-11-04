PREFIX ?= $(HOME)/.local

all: compile nvimi install

compile: nvim2.cpp
	g++ nvim2.cpp -o nvim2

nvimi: nvimi.cpp
	g++ nvimi.cpp -o nvimi

install: nvim2 nvimi
	install -Dm755 nvim2 $(PREFIX)/bin/nvim2
	install -Dm755 nvimi $(PREFIX)/bin/nvimi

uninstall:
	rm -f $(PREFIX)/bin/nvim2
	rm -f $(PREFIX)/bin/nvimi

clear:
	rm nvim2

.PHONY: compile clear nvimi install uninstall


