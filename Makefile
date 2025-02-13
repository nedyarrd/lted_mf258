CC=gcc
#CC_MIPSEL=mipsel-linux-gnu-gcc-12
CFLAGS=-I. -g -flto -fuse-linker-plugin
CFLAGS=-I. -g -flto -fuse-linker-plugin -Os -fomit-frame-pointer --static

lted: lted.o communication.o myll.o myudp.o at-commands.o
	$(CC) -fuse-linker-plugin -o lted lted.o communication.o myll.o myudp.o at-commands.o
	rm -rf *.o
	cp lted lted-strip
	mipsel-openwrt-linux-strip lted-strip

#CC=mipsel-linux-gnu-gcc-12
lted_mipsel $(CC_MIPSEL): lted.o communication.o myll.o myudp.o at-commands.o
	$(CC_MIPSEL)  -O3 -fuse-linker-plugin -o lted lted.o communication.o myll.o myudp.o at-commands.o
	rm -rf *.o
	cp lted lted-strip
	mipsel-linux-gnu-strip lted-strip

install:
	install lted /bin/lted
	install -D etc/lte /etc/config/lte
	install bin/poe_restart /bin/poe_restart

clean:
	rm -rf *.o