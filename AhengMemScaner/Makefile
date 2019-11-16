CC = gcc
CFLAGS = -std=gnu99  -g -O2 -w  -I$(LUA_INC) 
SHARED = -fPIC --shared
 
TARGET = Memory.so
LUA_CLIB_PATH = clib
SYSROOT=/tmp/my-android-toolchain/sysroot/usr/include
 
# 引入lua头文件
LUA_INC ?= ./src

start: $(TARGET)

$(TARGET) : Memory.c common.h 	scanmem.h 	commands.h 	show_message.h 	  common.h  handlers.h interrupt.h  common.c  licence.h   show_message.c  sets.h	commands.c    ptrace.c          handlers.c       interrupt.c      maps.c     scanmem.c     scanroutines.c        sets.c    targetmem.c     value.c  | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

clean:
	rm -fr $(TARGET)
              
$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)
