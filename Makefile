PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g
SOURCES = easydir.c handerror.c implementation.c main.c
OBJECTS = $(SOURCES:.c=.o)
BASH = lpass_copy.sh
EXECUTABLE = lpass

.PHONY: all clean install uninstall

all: $(EXECUTABLE)

clean:
	@rm -rf $(EXECUTABLE) $(OBJECTS)

$(OBJECTS):
	@$(CC) -c $(CFLAGS) $(SOURCES)

$(EXECUTABLE): $(OBJECTS)
	@$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJECTS)

install: all
	@echo installing files to $(PREFIX)
	@install $(EXECUTABLE) $(PREFIX)
	@chmod 755 $(PREFIX)/$(EXECUTABLE)
	@install $(BASH) $(PREFIX)
	@chmod 755 $(PREFIX)/$(BASH)

uninstall:
	@echo removing files from $(PREFIX)
	@rm -rf $(PREFIX)/$(EXECUTABLE)
	@rm -rf $(PREFIX)/$(BASH)