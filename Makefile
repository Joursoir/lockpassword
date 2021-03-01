PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g \
	$(shell gpgme-config --cflags --libs) #-DDEBUG
ifdef DISPLAY
	CFLAGS += -lX11 -DDISPLAY
endif
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
	@install -m 755 -v $(EXECUTABLE) $(PREFIX)
	@echo installing man page
	@cat $(MAN_SOURCES) | gzip > $(MAN_OBJECTS)
	@install $(MAN_OBJECTS) $(MAN_PATH)

uninstall:
	@echo removing files from $(PREFIX)
	@echo deleting man page
	@rm -rf \
		$(PREFIX)/$(EXECUTABLE) \
		$(MAN_PATH)/$(MAN_OBJECTS)