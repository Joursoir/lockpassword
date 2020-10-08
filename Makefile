VERSION = 1.01
PREFIX = /usr/local/bin
CC = gcc
CFLAGS = -Wall -g
SOURCES = easydir.c handerror.c main.c
OBJECTS = $(SOURCES:.c=.o)
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
	@echo installing file to $(PREFIX)
	#@mkdir -p $(PREFIX)
	#@cp -f $(EXECUTABLE) $(PREFIX)
	@install $(EXECUTABLE) $(PREFIX)
	@chmod 755 $(PREFIX)/$(EXECUTABLE)

uninstall:
	@echo removing file from $(PREFIX)
	@rm -rf $(PREFIX)/$(EXECUTABLE)