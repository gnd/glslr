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
  #CFLAGS+=-g
  CFLAGS+=-O2 -s
endif
CFLAGS += -DPLATFORM_LINUX $(shell pkg-config --cflags glfw3 glu gl) 

LIBS+=-lGLESv2
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

TARGET=glslr

SOURCES =main.c
SOURCES+=glslr.c
SOURCES+=graphics.c
SOURCES+=v4l2.c
SOURCES+=v4l2_controls.c
SOURCES+=sony.c

OBJECTS=$(subst .c,.o, $(SOURCES))


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS)

clean:
	rm -f *~
	rm -f $(OBJECTS) $(TARGET)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDE) -c $<
