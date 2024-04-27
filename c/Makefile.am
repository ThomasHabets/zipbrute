AUTOMAKE_OPTIONS=foreign subdir-objects
DISTCLEANFILES=*~

include_HEADERS=src/zipbrute.h

bin_PROGRAMS=zipbrute pwgen

pwgen_SOURCES=\
src/pwgen.c \
src/pwgen_inc.c \
src/pwgen_dict.c

zipbrute_SOURCES=\
src/crc.c \
src/nopvm_lib.c \
src/nopvm_main.c \
src/pwgen_dict.c \
src/pwgen_inc.c \
src/zip.c \
src/zipbrute.c \
src/zipbrute_global.c
