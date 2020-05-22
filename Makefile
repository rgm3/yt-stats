# This is not a full-blown Makefile.
# See also https://github.com/arduino/arduino-pro-ide/releases


SKETCH_DIR := $(PWD)
NAME := $(shell basename $(PWD))
INO := $(addsuffix .ino,$(NAME))

# Adafruit Feather HUZZAH
#   arduino-cli board listall
#   arduino-cli board details esp8266:esp8266:huzzah
BOARD := esp8266:esp8266:huzzah
BOARD_OPTIONS := :xtal=80,baud=460800,eesz=4M3M

# Use the first listed "Serial Port (USB)"
PORT := $(shell arduino-cli board list | awk '/dev.*Serial Port.*USB/{print $$1; exit;}')

OBJ_BASE = $(NAME).$(subst :,.,$(BOARD))
OBJS = $(OBJ_BASE).bin $(OBJ_BASE).elf


all: format $(OBJS) secrets.h.example

format: $(INO) 
	@clang-format -i --style=file $(addprefix ",$(addsuffix ",$^))

$(OBJS): $(INO) secrets.h
	arduino-cli compile --fqbn $(BOARD)$(BOARD_OPTIONS) $(SKETCH_DIR)

secrets.h.example: secrets.h
	@echo "// Rename this file to 'secrets.h' and customize" > secrets.h.example
	@sed 's/"\(.*\)"/"12345678"/' secrets.h >> secrets.h.example

upload: $(OBJS)
	arduino-cli upload -p $(PORT) --fqbn $(BOARD)$(BOARD_OPTIONS) $(SKETCH_DIR)

clean:
	@rm -f ./*.hex ./*.elf ./*.bin ./*.example


.PHONY: clean format upload
