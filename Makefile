ifeq ($(arch), win) # windows
ifeq ($(bits), 64)
BITS_FLAG       := -m64
TARGET_LIB      := lib/libadbclient64.dll

CC              := x86_64-w64-mingw32-gcc 
CXX             := x86_64-w64-mingw32-g++
LD              := x86_64-w64-mingw32-gcc
STP             := x86_64-w64-mingw32-strip
CFLAGS          += -I/usr/x86_64-w64-mingw32/include
else
BITS_FLAG       := -m32
TARGET_LIB      := lib/libadbclient32.dll

CC              := i686-w64-mingw32-gcc 
CXX             := i686-w64-mingw32-g++
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
TARGET_LIB      := lib/libadbclient64.so
else
BITS_FLAG       := -m32
TARGET_LIB      := lib/libadbclient32.so
endif
CC              := gcc -fPIC
CXX             := g++ -fPIC
LD              := g++ -fPIC
STP             := strip
endif

#JAVAHOME        := /usr/lib/jvm/java-6-openjdk-amd64
JAVAHOME        += /usr/lib/jvm/java-6-sun-1.6.0.24
CFLAGS          += $(BITS_FLAG) -Wall
CPPFLAGS        += $(BITS_FLAG) -g -I$(JAVAHOME)/include
LDFLAGS         += $(BITS_FLAG) -shared
OBJS            := adbclient.o status.o syslog.o file_sync_client.o \
                   sync.o zipfile.o centraldir.o adbClientJni.o log.o

all:$(OBJS)
	$(LD) $(LDFLAGS) -o $(TARGET_LIB) $(OBJS) $(AFLAG)
	$(STP) $(TARGET_LIB)
	rm -f *.o *.d

%.o:%.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $*.o

%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $*.o

#adbclient.o:adbclient.c
	#$(CC) -c $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f *.o *.d
	rm -rf lib/*.so
	rm -rf lib/*.dll
