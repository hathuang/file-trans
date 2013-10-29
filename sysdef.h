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

#define  lstat    stat   /* no symlinks on Win32 */
#define  S_ISLNK(m)   0   /* no symlinks on Win32 */

#ifndef Win32
#define Win32 
#endif

/* shutdown() how types */
#ifndef SD_RECEIVE
#define SD_RECEIVE      0x00
#define SD_SEND         0x01
#define SD_BOTH         0x02
#endif

#ifndef SHUT_RDWR
#define SHUT_RDWR	SD_BOTH	
#define SHUT_WR         SD_SEND         
#define SHUT_RD         SD_RECEIVE
#endif

#define ADB_DIR_SEP_STR "\\"
#define ADB_DIR_SEP_CHR '\\'
static __inline__ char *adb_dirstop(const char* path)
{
    char *p  = strrchr(path, '/');
    char *p2 = strrchr(path, '\\');

    if (!p)
        p = p2;
    else if (p2 && p2 > p)
        p = p2;

    return p;
}
/* Windows End */
#else

/* Linux */
#define OS_LINUX
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
 
#define ADB_DIR_SEP_STR "/"
#define ADB_DIR_SEP_CHR '/'
#define adb_dirstop(x) strrchr((x), '/')

__asm__(".symver memcpy,memcpy@GLIBC_2.2.5");

/* Linux End */
#endif

#define ADB_CLOSE_SOCK(sock, flag)	do {shutdown(sock, flag); close(sock);} while (0)

#endif /* "sysdef.h" */
