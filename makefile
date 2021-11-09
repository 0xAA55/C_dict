CC=gcc
LD=gcc
AR=gcc-ar
RANLIB=gcc-ranlib
CFLAGS=-O3 -flto -ffat-lto-objects

OBJS=dict.o dictcfg.o logprintf.o

all: libcdict.a

libcdict.a: $(OBJS)
	$(AR) rc $@ $+
	$(RANLIB) $@

clean:
	rm *.o *.a
