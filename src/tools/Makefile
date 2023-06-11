all:    mkfs.sfs

CFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -I../kernel
mkfs.sfs:   bitmap.c bitmap.h mkfs.c ../kernel/sfs.h
		gcc -g $(CFLAGS) -o mkfs.sfs bitmap.c mkfs.c

clean:
	rm 	mkfs.sfs
