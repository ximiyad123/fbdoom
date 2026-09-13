CFLAGS ?= -O2 -Wall -Wextra -std=c11

CPPFLAGS += \
	-Iinclude \
	-Isrc/fbdoom \
	-Isrc/fbprinter/include

# ------------------------------------------------------------
# Architecture / compiler
# ------------------------------------------------------------

ifeq ($(ARCH),arm64)
	override CC := aarch64-linux-gnu-gcc
	ARCH_DIR := arm64
else ifeq ($(ARCH),arm)
	override CC := arm-linux-gnueabihf-gcc
	ARCH_DIR := arm
else
	$(error Unsupported ARCH='$(ARCH)'. Use ARCH=arm64 or ARCH=arm)
endif


# ------------------------------------------------------------
# Build directories
# ------------------------------------------------------------

BUILD_DIR := build/arch/$(ARCH_DIR)

DYNAMIC_DIR := $(BUILD_DIR)/dynamic
STATIC_DIR  := $(BUILD_DIR)/static

DYNAMIC_TARGET := $(DYNAMIC_DIR)/fbdoom
STATIC_TARGET  := $(STATIC_DIR)/fbdoom


# ------------------------------------------------------------
# Sources
# ------------------------------------------------------------

SOURCES := \
	src/main.c \
	src/fbdoom/i_video_fbdoom.c \
	src/fbdoom/i_input_fbdoom.c


# ------------------------------------------------------------
# fbprinter
# ------------------------------------------------------------

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

DYNAMIC_LIB := \
	$(DYNAMIC_DIR)/libfbprinter.so


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

	@mkdir -p $(DYNAMIC_DIR)

	cp $(FBPRINTER_DIR)/build/dynamic/libfbprinter.so \
		$(DYNAMIC_LIB)

	@echo
	@echo "libfbprinter:"
	@echo "  $(DYNAMIC_LIB)"
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
	@echo " Output       : $(BUILD_DIR)"
	@echo "========================================"

ifeq ($(MODE),static)

	@mkdir -p $(STATIC_DIR)

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
		-o $(STATIC_TARGET)

	@echo
	@echo "FBDOOM:"
	@echo "  $(STATIC_TARGET)"
	@echo

else

	@mkdir -p $(DYNAMIC_DIR)

	$(CC) \
		$(CFLAGS) \
		$(CPPFLAGS) \
		$(SOURCES) \
		-L$(DYNAMIC_DIR) \
		-Wl,-rpath,'$$ORIGIN' \
		-lfbprinter \
		-o $(DYNAMIC_TARGET)

	@echo
	@echo "FBDOOM:"
	@echo "  $(DYNAMIC_TARGET)"
	@echo

endif


# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

clean:
	rm -rf build

	$(MAKE) -C $(FBPRINTER_DIR) clean
