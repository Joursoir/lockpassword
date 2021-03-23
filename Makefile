PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g #-DDEBUG
LIBS = $(shell gpgme-config --cflags --libs)
MAN_PATH = /usr/share/man/man1
SOURCES = src/main.c \
	src/exec-cmd.c \
	src/routines.c \
	src/easydir.c \
	src/r-gpgme.c \
	src/tree.c \
	src/xstd.c
ifdef DISPLAY
	LIBS += -lX11
	CFLAGS += -DDISPLAY
	SOURCES += src/r-x11.c
endif
OBJECTS = ${SOURCES:.c=.o}
MAN_SOURCES = man/lpass.1
MAN_OBJECTS = lpass.1.gz
EXECUTABLE = lpass

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

clean:
	@rm -rf $(EXECUTABLE) $(OBJECTS) $(MAN_OBJECTS)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(CFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJECTS)

%.o: %.c
	@$(CC) $(CFLAGS) $(LIBS) -c $< -o $@

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