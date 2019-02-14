# GLSLR - lightweight livecoding environment
TARGET=glslr
DEBUG=no
CC=gcc

CFLAGS =-Wall
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

LIBS+=-lglfw
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
LIBS+=-lm
LIBS+=-lpthread
LIBS+=-ldl
LIBS+=-lcurl
LIBS+=-ljpeg

SOURCES =main.c
SOURCES+=glslr.c
SOURCES+=graphics.c
SOURCES+=v4l2.c
SOURCES+=v4l2_controls.c
SOURCES+=sony.c

OBJECTS=$(subst .c,.o, $(SOURCES))

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
