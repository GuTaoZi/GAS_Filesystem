all:    mkfs.gas

CFLAGS = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -I../kern
mkfs.gas:   bitmap.c bitmap.h mkfs.c ../kernel/gas.h
		gcc -g $(CFLAGS) -o mkfs.gas bitmap.c mkfs.c

clean:
	rm 	mkfs.gas
