CFLAGS ?= -O2 -Wall -Wextra -std=c11

CPPFLAGS += \
	-I/opt/fbprinter-deps/include \
	-I/opt/fbprinter-deps/include/SDL2 \
	-Iinclude \
	-Isrc/fbdoom \
	-Isrc/fbprinter/include \
	-Isrc/chocolate-doom \
	-Isrc/chocolate-doom/src \
	-Isrc/chocolate-doom/src/doom \
	-Isrc/chocolate-doom/textscreen \
	-Isrc/chocolate-doom/opl \
	-Isrc/chocolate-doom/pcsound
	
# ------------------------------------------------------------
# Architecture / compiler
# ------------------------------------------------------------

ifeq ($(ARCH),arm64)
	override CC := aarch64-linux-gnu-gcc
	ARCH_DIR := arm64
	CHOCOLATE_HOST := aarch64-linux-gnu
else ifeq ($(ARCH),arm)
	override CC := arm-linux-gnueabihf-gcc
	ARCH_DIR := arm
	CHOCOLATE_HOST := arm-linux-gnueabihf
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
# Chocolate Doom
# ------------------------------------------------------------

CHOCOLATE_DOOM_DIR := src/chocolate-doom

DOOM_DIR := $(CHOCOLATE_DOOM_DIR)/src/doom

DOOM_LIB := \
	$(DOOM_DIR)/libdoom.a


# ------------------------------------------------------------
# Chocolate Doom common objects
#
# We reuse Chocolate Doom's own generated objects.
#
# Intentionally excluded:
#
#   i_main.o   -> FBDOOM has its own main.c
#   i_input.o  -> replaced by i_input_fbdoom.c
#   i_video.o  -> replaced by i_video_fbdoom.c
#   i_endoom.o -> requires Textscreen
#   net_gui.o  -> requires Textscreen
# ------------------------------------------------------------

CHOCOLATE_COMMON_OBJECTS := \
	i_system.o \
	m_argv.o \
	m_misc.o \
	aes_prng.o \
	d_event.o \
	d_iwad.o \
	d_loop.o \
	d_mode.o \
	deh_str.o \
	gusconf.o \
	i_cdmus.o \
	i_flmusic.o \
	i_glob.o \
	i_joystick.o \
	i_musicpack.o \
	i_oplmusic.o \
	i_pcsound.o \
	i_sdlmusic.o \
	i_sdlsound.o \
	i_sound.o \
	i_timer.o \
	i_videohr.o \
	midifallback.o \
	midifile.o \
	mus2mid.o \
	m_bbox.o \
	m_cheat.o \
	m_config.o \
	m_controls.o \
	m_fixed.o \
	net_client.o \
	net_common.o \
	net_dedicated.o \
	net_io.o \
	net_loop.o \
	net_packet.o \
	net_petname.o \
	net_query.o \
	net_sdl.o \
	net_server.o \
	net_structrw.o \
	p_rejectpad.o \
	sha1.o \
	memio.o \
	tables.o \
	v_diskicon.o \
	v_video.o \
	w_checksum.o \
	w_main.o \
	w_wad.o \
	w_file.o \
	w_file_stdc.o \
	w_file_posix.o \
	w_file_win32.o \
	w_merge.o \
	z_zone.o \
	deh_io.o \
	deh_main.o \
	deh_mapping.o \
	deh_text.o

CHOCOLATE_COMMON_OBJECT_PATHS := \
	$(addprefix $(CHOCOLATE_DOOM_DIR)/src/,$(CHOCOLATE_COMMON_OBJECTS))


# ------------------------------------------------------------
# Chocolate Doom support libraries
# ------------------------------------------------------------

OPL_LIB := \
	$(CHOCOLATE_DOOM_DIR)/opl/libopl.a

PCSOUND_LIB := \
	$(CHOCOLATE_DOOM_DIR)/pcsound/libpcsound.a


# ------------------------------------------------------------
# FBDOOM sources
# ------------------------------------------------------------

SOURCES := \
	src/main.c \
	src/fbdoom/i_video_fbdoom.c \
	src/fbdoom/i_input_fbdoom.c \
	src/fbdoom/i_platform_fbdoom.c


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

.PHONY: \
	all \
	dynamic \
	static \
	fbprinter \
	chocolate-doom \
	chocolate-common \
	chocolate-support \
	fbdoom \
	clean


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
# Build Chocolate Doom common objects
# ------------------------------------------------------------

chocolate-common:
	@echo "========================================"
	@echo " Building Chocolate Doom common layer"
	@echo "========================================"

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/src \
		$(CHOCOLATE_COMMON_OBJECTS) \
		CC=$(CC) \
		AR=$(CHOCOLATE_HOST)-ar \
		RANLIB=$(CHOCOLATE_HOST)-ranlib


# ------------------------------------------------------------
# Build Chocolate Doom support libraries
# ------------------------------------------------------------

chocolate-support:
	@echo "========================================"
	@echo " Building Chocolate Doom support libs"
	@echo "========================================"

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/opl \
		CC=$(CC) \
		AR=$(CHOCOLATE_HOST)-ar \
		RANLIB=$(CHOCOLATE_HOST)-ranlib

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/pcsound \
		CC=$(CC) \
		AR=$(CHOCOLATE_HOST)-ar \
		RANLIB=$(CHOCOLATE_HOST)-ranlib


# ------------------------------------------------------------
# Build Chocolate Doom engine
# ------------------------------------------------------------

chocolate-doom: chocolate-common chocolate-support
	@echo "========================================"
	@echo " Building Chocolate Doom engine"
	@echo "========================================"

	$(MAKE) -C $(DOOM_DIR) \
		CC=$(CC) \
		AR=$(CHOCOLATE_HOST)-ar \
		RANLIB=$(CHOCOLATE_HOST)-ranlib


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

fbdoom: chocolate-doom fbprinter

	@echo "========================================"
	@echo " Building FBDOOM"
	@echo "========================================"
	@echo " Architecture : $(ARCH)"
	@echo " Compiler     : $(CC)"
	@echo " Mode         : $(MODE)"
	@echo " Output       : $(BUILD_DIR)"
	@echo " Doom engine  : $(DOOM_LIB)"
	@echo "========================================"

ifeq ($(MODE),static)

	@mkdir -p $(STATIC_DIR)

	$(CC) \
		-static \
		$(CFLAGS) \
		$(CPPFLAGS) \
		$(SOURCES) \
		$(CHOCOLATE_COMMON_OBJECT_PATHS) \
		$(DOOM_LIB) \
		$(FBPRINTER_LIB_OBJECTS) \
		-L/opt/fbprinter-deps/lib \
		-lSDL2_mixer \
		-lSDL2_net \
		-lSDL2 \
		-lpng16 \
		-ljpeg \
		-lgif \
		-lz \
		-lpthread \
		-ldl \
		-lm \
		$(PCSOUND_LIB) \
		$(OPL_LIB) \
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
		$(CHOCOLATE_COMMON_OBJECT_PATHS) \
		$(DOOM_LIB) \
		-L$(DYNAMIC_DIR) \
		-Wl,-rpath,'$$ORIGIN' \
		-lfbprinter \
		-L/opt/fbprinter-deps/lib \
		-lSDL2_mixer \
		-lSDL2_net \
		-lSDL2 \
		-lpthread \
		-ldl \
		-lm \
		$(PCSOUND_LIB) \
		$(OPL_LIB) \
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

	@echo "Cleaning Chocolate Doom objects..."

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/src clean \
		>/dev/null 2>&1 || true

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/src/doom clean \
		>/dev/null 2>&1 || true

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/opl clean \
		>/dev/null 2>&1 || true

	$(MAKE) -C $(CHOCOLATE_DOOM_DIR)/pcsound clean \
		>/dev/null 2>&1 || true
