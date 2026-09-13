CFLAGS ?= -O2 -Wall -Wextra -std=c11

CPPFLAGS += \
	-Iinclude \
	-Isrc/fbdoom \
	-Isrc/fbprinter/include

TARGET := build/bin/fbdoom

SOURCES := \
	src/main.c \
	src/fbdoom/i_video_fbdoom.c \
	src/fbdoom/i_input_fbdoom.c

FBPRINTER_DIR := src/fbprinter

FBPRINTER_OBJ_DIR := \
	$(FBPRINTER_DIR)/build/obj

FBPRINTER_LIB_OBJECTS := \
	$(FBPRINTER_OBJ_DIR)/libfbprinter.o \
	$(FBPRINTER_OBJ_DIR)/framebuffer.o \
	$(FBPRINTER_OBJ_DIR)/renderer.o \
	$(FBPRINTER_OBJ_DIR)/image.o \
	$(FBPRINTER_OBJ_DIR)/text.o \
	$(FBPRINTER_OBJ_DIR)/png.o \
	$(FBPRINTER_OBJ_DIR)/jpg.o \
	$(FBPRINTER_OBJ_DIR)/gif.o \
	$(FBPRINTER_OBJ_DIR)/ini_parser.o


# ------------------------------------------------------------
# Architecture / compiler
# ------------------------------------------------------------

ifeq ($(ARCH),arm64)
	override CC := aarch64-linux-gnu-gcc
else ifeq ($(ARCH),arm)
	override CC := arm-linux-gnueabihf-gcc
else
	$(error Unsupported ARCH='$(ARCH)'. Use ARCH=arm64 or ARCH=arm)
endif


# ------------------------------------------------------------
# Phony targets
# ------------------------------------------------------------

.PHONY: all dynamic static fbprinter fbdoom clean


# ------------------------------------------------------------
# Default
# ------------------------------------------------------------

all: dynamic


# ------------------------------------------------------------
# Dynamic
# ------------------------------------------------------------

dynamic:
	$(MAKE) ARCH=$(ARCH) MODE=dynamic fbdoom


# ------------------------------------------------------------
# Static
# ------------------------------------------------------------

static:
	$(MAKE) ARCH=$(ARCH) MODE=static fbdoom


# ------------------------------------------------------------
# Build fbprinter
# ------------------------------------------------------------

fbprinter:
	@echo "========================================"
	@echo " Building libfbprinter"
	@echo "========================================"

ifeq ($(MODE),static)

	$(MAKE) -C $(FBPRINTER_DIR) \
		ARCH=$(ARCH) \
		static

else

	$(MAKE) -C $(FBPRINTER_DIR) \
		ARCH=$(ARCH) \
		dynamic

	@mkdir -p build/lib

	cp $(FBPRINTER_DIR)/build/dynamic/libfbprinter.so \
		build/lib/libfbprinter.so

	@echo
	@echo "libfbprinter:"
	@echo "  build/lib/libfbprinter.so"
	@echo

endif


# ------------------------------------------------------------
# Build FBDOOM
# ------------------------------------------------------------

fbdoom: fbprinter

	@echo "========================================"
	@echo " Building FBDOOM"
	@echo "========================================"
	@echo " Architecture : $(ARCH)"
	@echo " Compiler     : $(CC)"
	@echo " Mode         : $(MODE)"
	@echo "========================================"

	@mkdir -p build/bin

ifeq ($(MODE),static)

	$(CC) \
		-static \
		$(CFLAGS) \
		$(CPPFLAGS) \
		$(SOURCES) \
		$(FBPRINTER_LIB_OBJECTS) \
		-L/opt/fbprinter-deps/lib \
		-lpng \
		-ljpeg \
		-lgif \
		-lz \
		-lm \
		-o $(TARGET)

else

	$(CC) \
		$(CFLAGS) \
		$(CPPFLAGS) \
		$(SOURCES) \
		-Lbuild/lib \
		-Wl,-rpath,'$$ORIGIN/../lib' \
		-lfbprinter \
		-o $(TARGET)

endif

	@echo
	@echo "FBDOOM:"
	@echo "  $(TARGET)"
	@echo


# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf build

	$(MAKE) -C $(FBPRINTER_DIR) clean
