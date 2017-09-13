# glslr GLSL livecoder

TARGET=glslr
VERSION=0.1
DEBUG=yes

all:
	make -C src $@
	cp -fu src/$(TARGET) ./

clean:
	make -C src clean
	rm -f $(TARGET)

archive:
	hg archive -t tgz glslr-$(VERSION)-src.tar.gz -p glslr-$(VERSION)-src
