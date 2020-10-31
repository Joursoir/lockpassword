PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g
MAN_PATH = /usr/share/man/man1
SOURCES = src/easydir.c src/handerror.c src/implementation.c src/main.c
OBJECTS = easydir.o handerror.o implementation.o main.o
MAN_SOURCES = man/lpass.1
MAN_OBJECTS = lpass.1.gz
BASH = lpass_copy.sh
EXECUTABLE = lpass

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

clean:
	@rm -rf $(EXECUTABLE) $(OBJECTS)
	@rm -rf $(MAN_OBJECTS)

$(OBJECTS):
	@$(CC) -c $(CFLAGS) $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJECTS)

install: all
	@echo installing files to $(PREFIX)
	@install $(EXECUTABLE) $(PREFIX) && chmod 755 $(PREFIX)/$(EXECUTABLE)
	@install $(BASH) $(PREFIX) && chmod 755 $(PREFIX)/$(BASH)
	@echo installing man page
	@cat $(MAN_SOURCES) | gzip > $(MAN_OBJECTS)
	@install $(MAN_OBJECTS) $(MAN_PATH)

uninstall:
	@echo removing files from $(PREFIX)
	@echo deleting man page
	@rm -rf \
		$(PREFIX)/$(EXECUTABLE) \
		$(PREFIX)/$(BASH) \
		$(MAN_PATH)/$(MAN_OBJECTS)