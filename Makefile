IDIR=include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR=lib

LIBS=

LIB_OBJ=extfs.o extfs_debug.o extfs_glue.o

all: libextfs.a examples

examples: print_passwd

print_passwd: obj/print_passwd.o libextfs.a
	$(CC) $(CFLAGS) -o examples/$@ $+ $(LIBS)

obj/print_passwd.o: examples/print_passwd.c
	$(CC) $(CFLAGS) -c $< -o $@

libextfs.a: $(patsubst %,$(ODIR)/%,$(LIB_OBJ))
	ar rcs $@ $(patsubst %.o, %.o, $+)

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
	rm -f libextfs.a
	rm -f examples/print_passwd