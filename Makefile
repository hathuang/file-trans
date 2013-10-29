ifeq ($(arch), win) # windows
ifeq ($(bits), 64)
BITS_FLAG       := -m64
ifeq ($(target), server)
TARGET_BIN      := server-64.exe
else
TARGET_BIN      := client-64.exe
endif

CC              := x86_64-w64-mingw32-gcc 
LD              := x86_64-w64-mingw32-gcc
STP             := x86_64-w64-mingw32-strip
CFLAGS          += -I/usr/x86_64-w64-mingw32/include
else
BITS_FLAG       := -m32
ifeq ($(target), server)
TARGET_BIN      := server-32.exe
else
TARGET_BIN      := client-32.exe
endif

CC              := i686-w64-mingw32-gcc 
LD              := i686-w64-mingw32-gcc
STP             := i686-w64-mingw32-strip
CFLAGS          += -I/usr/i686-w64-mingw32/include
endif

AFLAG           += -lwsock32
AFLAG           += -lws2_32
CFLAGS          += -MD

else               # linux

ifeq ($(bits), 64)
BITS_FLAG       := -m64
ifeq ($(target), server)
TARGET_BIN      := server-64
else
TARGET_BIN      := client-64
endif
else
BITS_FLAG       := -m32
ifeq ($(target), server)
TARGET_BIN      := server-32
else
TARGET_BIN      := client-32
endif
endif
CC              := gcc
LD              := gcc
STP             := strip
endif

CFLAGS          += $(BITS_FLAG) -Wall
LDFLAGS         += $(BITS_FLAG)
OBJS            := main.o

all:$(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET_BIN) $(OBJS) $(AFLAG)
	$(STP) $(TARGET_BIN)
	rm -f *.o *.d

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $*.o

#adbclient.o:adbclient.c
	#$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.d
	rm -f server* client*
