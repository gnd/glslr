# GLSLR - lightweight livecoding environment
TARGET=glslr
LINUX=yes
MACOS=no
VIDEO=no
DEBUG=no
CC=gcc

# Detect the platform first
ifeq ($(OS),Windows_NT)
    echo "Windows platforms not supported"
	exit
else
    UNAME_S := $(shell uname -s)
    ifeq (Linux,$(UNAME_S))
        CFLAGS = -DLINUX
    endif
    ifeq (Darwin,$(UNAME_S))
        MACOS=yes
        LINUX=no
        VIDEO=no
        CFLAGS = -DOSX
    endif
endif

# Is video allowed
ifeq (yes, $(VIDEO))
    CFLAGS += -DVIDEO
endif

# Set some basic flags
CFLAGS+=-Wall
CFLAGS+=-Wextra
CFLAGS+=-Wwrite-strings
CFLAGS+=-Wpointer-arith
CFLAGS+=-Wfloat-equal
CFLAGS+=-Wcast-align
ifeq (yes, $(DEBUG))
  CFLAGS+=-g
else
  CFLAGS+=-O2 -s
endif

ifeq ($(PREFIX),)
    PREFIX := /usr
endif

# Load libraries according to the platform
# Default libs
LIBS+=-lglfw
LIBS+=-lm
LIBS+=-lpthread
LIBS+=-ldl
LIBS+=-lcurl
LIBS+=-ljpeg
# Linux
ifeq (yes, $(LINUX))
  LIBS+=-lGL
  LIBS+=-lX11
  LIBS+=-lXrandr
  LIBS+=-lXi
  LIBS+=-lXxf86vm
  LIBS+=-lXcursor
  LIBS+=-lXinerama
  LIBS+=-lXrender
  LIBS+=-lXext
  LIBS+=-lXdamage
endif
# Mac OS
ifeq (yes, $(MACOS))
  LIBS+=-framework OpenGL
  LIBS+=-framework Cocoa
  LIBS+=-framework IOkit
  LIBS+=lglew
endif

# Define some source files
SOURCES =main.c
SOURCES+=glslr.c
SOURCES+=graphics.c
SOURCES+=sony.c
ifeq (yes, $(VIDEO))
  SOURCES+=v4l2.c
  SOURCES+=v4l2_controls.c
endif

# Define some objects
OBJECTS=$(subst .c,.o, $(SOURCES))

# Define compilation targets
all: $(TARGET) install

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

install:
	install -D $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -f *~
	rm -f $(OBJECTS) $(TARGET)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<
