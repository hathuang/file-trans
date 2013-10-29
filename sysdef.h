#ifndef SYSDEF_H
#define SYSDEF_H

#ifdef _WIN32

/* Windows */
#define OS_WINDOWS
#include <ctype.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <process.h>
#include <wtypes.h>
#include <winnls.h>
#include <io.h>

#define lstat stat   /* no symlinks on Win32 */
#define mkdir(path, mode) mkdir(path)

#ifndef Win32
#define Win32 
#endif

/* Windows End */
#else

/* Linux */
#define OS_LINUX
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");*/

/* Linux End */
#endif

#endif /* "sysdef.h" */
