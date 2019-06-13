CC = gcc
CFLAGS = -O2 -Wall -I .

# This flag includes the Pthreads library on a Linux box.
# Others systems will probably require something different.
LIB = -lpthread

all: tiny cgi

tiny: tiny.c csapp.o
	$(CC) $(CFLAGS) -o tiny tiny.c csapp.o $(LIB)

csapp.o:
	$(CC) $(CFLAGS) -c csapp.c

cgi:
	(cd cgi-bin; make)

# Generate the files used to test max cached file size of 100Kib
cachefiles:
	dd if=/dev/urandom of=1kib.dat bs=1 count=1024           # OK 
	dd if=/dev/urandom of=100kibminus.dat bs=1 count=102399  # OK (one byte smaller than legal size)
	dd if=/dev/urandom of=100kibexact.dat bs=1 count=102400  # OK (exact legal size)
	dd if=/dev/urandom of=100kibplus.dat bs=1 count=102401   # not OK (one byte larger than legal size)
	dd if=/dev/urandom of=200kib.dat bs=1 count=204800       # not OK (twice the legal size)

clean:
	rm -f *.o tiny *~
	(cd cgi-bin; make clean)

