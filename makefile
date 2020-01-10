# GLSLR - lightweight livecoding environment
TARGET=glslr
VERSION=0.3
DEBUG=yes

all:
	make -C src $@
	cp -f src/$(TARGET) ./

install:
	make -C src install

clean:
	make -C src clean
	rm -f $(TARGET)

archive:
	hg archive -t tgz glslr-$(VERSION)-src.tar.gz -p glslr-$(VERSION)-src
