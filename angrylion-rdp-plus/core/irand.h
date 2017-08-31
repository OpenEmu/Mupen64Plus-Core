#pragma once

#include "common.h"

#include <stdint.h>

static TLS int32_t iseed = 1;

static STRICTINLINE int32_t irand()
{
    iseed *= 0x343fd;
    iseed += 0x269ec3;
    return ((iseed >> 16) & 0x7fff);
}
