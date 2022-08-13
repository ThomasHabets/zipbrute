# $id$

CC=gcc
CFLAGS=-g #-O6 -fomit-frame-pointer  #-fprofile-arcs -ftest-coverage -fno-inline -pg
PVM_CFLAGS=-DPVM=1 $(CFLAGS)

CXX=g++
CXXFLAGS=$(CFLAGS)
GCJ=gcj

TARGETS=zipbrute pwgen pvmzipbrute

#----#
all: $(TARGETS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
#----#
pwgen.h: pwgen_inc.h
pwgen.o: pwgen.c pwgen.h
pwgen_inc.o: pwgen_inc.c pwgen_inc.h
pwgen_dict.o: pwgen_dict.c pwgen_dict.h
PWGEN_A_O=pwgen_inc.o pwgen_dict.o
PWGEN_O=pwgen.o pwgen.a
PWGEN_H=pwgen.h pwgen_inc.h pwgen_dict.h
pwgen.a: $(PWGEN_A_O)
	ar r pwgen.a $(PWGEN_A_O)
pwgen: $(PWGEN_O) $(PWGEN_H)
	$(CC) $(CFLAGS) -o $@ $(PWGEN_O)
#----#
zipbrute_i386.o zipbrute_i386.s: zipbrute_arch.c zipbrute.c
	$(CC) $(CFLAGS) -DZB_ARCH=I386 -march=i386 -c -o $@ $<
#	$(CC) $(CFLAGS) -save-temps -DZB_ARCH=I386 -march=i386 -c -o $@ $<
#	mv zipbrute_arch.s zipbrute_i386.s
#	rm zipbrute_arch.i
zipbrute_i486.o zipbrute_i486.s: zipbrute_arch.c zipbrute.c
	$(CC) $(CFLAGS) -DZB_ARCH=I486 -march=i486 -c -o $@ $<
#	$(CC) $(CFLAGS) -save-temps -DZB_ARCH=I486 -march=i486 -c -o $@ $<
#	mv zipbrute_arch.s zipbrute_i486.s
#	rm zipbrute_arch.i
zipbrute_i586.o zipbrute_i586.s: zipbrute_arch.c zipbrute.c
	$(CC) $(CFLAGS) -DZB_ARCH=I586 -march=i586 -c -o $@ $<
#	$(CC) $(CFLAGS) -save-temps -DZB_ARCH=I586 -march=i586 -c -o $@ $<
#	mv zipbrute_arch.s zipbrute_i586.s
#	rm zipbrute_arch.i
zipbrute_i686.o zipbrute_i686.s: zipbrute_arch.c zipbrute.c
	$(CC) $(CFLAGS) -DZB_ARCH=I686 -march=i686 -c -o $@ $<
#	$(CC) $(CFLAGS) -save-temps -DZB_ARCH=I686 -march=i686 -c -o $@ $<
#	mv zipbrute_arch.s zipbrute_i686.s
#	rm zipbrute_arch.i
ZIPBRUTE_CORE=zip.o \
zipbrute_global.o \
zipbrute_i386.o \
zipbrute_i486.o \
zipbrute_i586.o \
zipbrute_i686.o
#----#
pvm_main.o: pvm_main.c
	$(CC) $(PVM_CFLAGS) -c -o $@ $<
pvm_lib.o: pvm_lib.c
	$(CC) $(PVM_CFLAGS) -c -o $@ $<
PVMZIPBRUTE_O=$(ZIPBRUTE_CORE) crc.o pvm_main.o pvm_lib.o pwgen.a
PVMZIPBRUTE_H=crc.h
pvmzipbrute: $(PVMZIPBRUTE_O) $(PVMZIPBRUTE_H)
	$(CC) $(PVM_CFLAGS) -o $@ $(PVMZIPBRUTE_O) -lpvm3
#----#
ZIPBRUTE_O=$(ZIPBRUTE_CORE) crc.o nopvm_main.o nopvm_lib.o pwgen.a
ZIPBRUTE_H=crc.h
zipbrute: $(ZIPBRUTE_O) $(ZIPBRUTE_H)
	$(CC) $(CFLAGS) -o $@ $(ZIPBRUTE_O) -lm
#----#
zipbrute.class: zipbrute.java
	$(GCJ) $(CFLAGS) -C $<
#----#
clean:
	rm -f *.o *.a $(TARGETS) gmon.out gmon.sum
