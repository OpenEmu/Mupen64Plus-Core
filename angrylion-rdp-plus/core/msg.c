#include "msg.h"

#ifdef WIN32
#include <Windows.h>
#endif

#include <stdarg.h>
#include <stdio.h>

#define MSG_BUFFER_LEN 256

void msg_error(const char * err, ...)
{
    char buf[MSG_BUFFER_LEN];
    va_list arg;
    va_start(arg, err);
    vsprintf(buf, err, arg);
#ifdef WIN32
    MessageBoxA(0, buf, "RDP: fatal error", MB_OK);
#else
    printf(buf);
#endif
    va_end(arg);
    exit(0);
}

void msg_warning(const char* err, ...)
{
    char buf[MSG_BUFFER_LEN];
    va_list arg;
    va_start(arg, err);
    vsprintf(buf, err, arg);
#ifdef WIN32
    MessageBoxA(0, buf, "RDP: warning", MB_OK);
#else
    printf(buf);
#endif
    va_end(arg);
}

void msg_debug(const char* err, ...)
{
    char buf[MSG_BUFFER_LEN];
    va_list arg;
    va_start(arg, err);
    vsprintf(buf, err, arg);
#ifdef WIN32
    OutputDebugStringA(buf);
#else
    printf(buf);
#endif
    va_end(arg);
}
