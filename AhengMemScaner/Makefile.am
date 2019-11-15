ACLOCAL_AMFLAGS = -I m4

SUBDIRS = test

if ENABLE_GUI
  SUBDIRS += po gui
endif

# '-O2 -g' are added by `configure`, unless the user overrides CFLAGS
AM_CFLAGS = -std=gnu99 -Wall


# soMemory
projectlibdir= $(includedir)/scanmem
projectlib_PROGRAMS=libmemory.so
libmemory_so_includedir=-I./src
libmemory_sodir=-I./
libmemory_so_HEADERS=common.h \
	scanmem.h \
	commands.h \
	show_message.h \
	lua.h \
	lauxlib.h
libmemory_so_SOURCES=Memory.c \
	common.h \
    common.c \
    show_message.c \
 	commands.c \
    ptrace.c \
    handlers.h \
    handlers.c \
    interrupt.h \
    interrupt.c \
    licence.h \
    maps.c \
    scanmem.c \
    scanroutines.c \
    sets.h \
    sets.c \
    targetmem.c \
    value.c 
#libmemory_so_LDADD = lua
libmemory_so_LDFLAGS= --shared -fpic -L./  -llua

# Misc
dist_man_MANS = scanmem.1
dist_doc_DATA = README

EXTRA_DIST = gpl-3.0.txt lgpl-3.0.txt
