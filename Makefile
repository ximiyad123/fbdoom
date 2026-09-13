CC ?= gcc

CFLAGS ?= -O2 -Wall -Wextra -std=c11
CPPFLAGS += -Iinclude -Isrc/fbdoom
LDFLAGS ?=

FBDOOM_SOURCES := \
	src/main.c \
	src/fbdoom/i_video_fbdoom.c \
	src/fbdoom/i_input_fbdoom.c

.PHONY: all clean

all: fbdoom

fbdoom:
	$(CC) $(CPPFLAGS) $(CFLAGS) \
		$(FBDOOM_SOURCES) \
		-Llib \
		-lfbprinter \
		-Wl,-rpath,'$$ORIGIN/lib' \
		$(LDFLAGS) \
		-o $@

clean:
	rm -f fbdoom
