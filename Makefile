PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g # -DDEBUG
MAN_PATH = /usr/share/man/man1
SOURCES = src/*.c
OBJECTS = *.o
MAN_SOURCES = man/lpass.1
MAN_OBJECTS = lpass.1.gz
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
	@echo installing man page
	@cat $(MAN_SOURCES) | gzip > $(MAN_OBJECTS)
	@install $(MAN_OBJECTS) $(MAN_PATH)

uninstall:
	@echo removing files from $(PREFIX)
	@echo deleting man page
	@rm -rf \
		$(PREFIX)/$(EXECUTABLE) \
		$(MAN_PATH)/$(MAN_OBJECTS)