# Constants
CC = gcc
NAME = server

# Code files
SOURCES := $(shell find . -iname '*.c')
OBJECTS := $(SOURCES:.c=.o)

$(NAME):	$(OBJECTS)
	$(CC) $(OBJECTS) $(LFLAGS) -o $(NAME)
	mv $(NAME) exe/$(NAME)

all: $(NAME)
.PHONY: clean
clean:
	rm -f exe/$(NAME) $(OBJECTS)