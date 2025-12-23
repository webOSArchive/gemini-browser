# Gemini Browser for webOS
# Cross-compilation Makefile for HP TouchPad

# Toolchain - Linaro GCC 4.8
LINARO_GCC ?= $(CURDIR)/toolchains/gcc-linaro
ARM_PREFIX = arm-linux-gnueabi
CC = $(LINARO_GCC)/bin/$(ARM_PREFIX)-gcc
STRIP = $(LINARO_GCC)/bin/$(ARM_PREFIX)-strip

# PalmPDK paths
PALM_PDK = /opt/PalmPDK
PDK_INCLUDE = $(PALM_PDK)/include
PDK_LIB = $(PALM_PDK)/device/lib

# OpenSSL 1.0.2p libraries (from com.nizovn.openssl)
# Runtime uses com.nizovn.openssl package on device
OPENSSL_LIB = $(CURDIR)/libs/openssl

# webOS runtime paths
APP_PATH = /media/cryptofs/apps/usr/palm/applications/org.webosarchive.geminibrowser
OPENSSL_RUNPATH = /media/cryptofs/apps/usr/palm/applications/org.webosarchive.geminibrowser/lib

# Compiler flags
CFLAGS = -Wall -Wextra -std=c11 -O2
CFLAGS += -march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp
CFLAGS += -I$(PDK_INCLUDE) -I$(PDK_INCLUDE)/SDL
CFLAGS += -DWEBOS

# Linker flags - search OpenSSL 1.0.2p first to get correct sonames
LDFLAGS = -L$(OPENSSL_LIB)
LDFLAGS += -L$(PDK_LIB)
LDFLAGS += -Wl,-rpath-link,$(OPENSSL_LIB)
LDFLAGS += -Wl,-rpath-link,$(PDK_LIB)
LDFLAGS += -Wl,-rpath,$(OPENSSL_RUNPATH)
LDFLAGS += -Wl,--allow-shlib-undefined

# Libraries
LIBS = -lSDL -lSDL_ttf -lSDL_image -lpdl -lssl -lcrypto

# Source files
SRC = src/main.c \
      src/gemini.c \
      src/document.c \
      src/render.c \
      src/ui.c \
      src/history.c \
      src/url.c \
      src/unicode.c

OBJ = $(SRC:.c=.o)
TARGET = gemini

.PHONY: all clean install package

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

strip: $(TARGET)
	$(STRIP) $(TARGET)

# Dependencies
src/main.o: src/main.c src/gemini.h src/document.h src/render.h src/ui.h src/history.h src/url.h
src/gemini.o: src/gemini.c src/gemini.h src/url.h
src/document.o: src/document.c src/document.h src/unicode.h
src/render.o: src/render.c src/render.h src/document.h
src/ui.o: src/ui.c src/ui.h src/render.h src/url.h
src/history.o: src/history.c src/history.h
src/url.o: src/url.c src/url.h
src/unicode.o: src/unicode.c src/unicode.h
