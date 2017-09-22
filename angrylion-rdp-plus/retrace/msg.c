#include "core/msg.h"
#include "core/version.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_BUFFER_LEN 256

void msg_error(const char * err, ...)
{
    printf("Error: ");

    va_list arg;
    va_start(arg, err);
    vprintf(err, arg);
    va_end(arg);

    printf("\n");

    exit(0);
}

void msg_warning(const char* err, ...)
{
    printf("Warning: ");

    va_list arg;
    va_start(arg, err);
    vprintf(err, arg);
    va_end(arg);

    printf("\n");
}

void msg_debug(const char* err, ...)
{
    printf("Debug: ");

    va_list arg;
    va_start(arg, err);
    vprintf(err, arg);
    va_end(arg);

    printf("\n");
}
