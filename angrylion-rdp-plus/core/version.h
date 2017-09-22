#pragma once

#define GIT_BRANCH "master"
#define GIT_TAG "v2.5.3-37-dirty"
#define GIT_COMMIT_HASH "12f0fe630fbeff46f9d3b63b245c74c6a6f0b032"
#define GIT_COMMIT_DATE "2017-09-22 12:50:55 -0500"

#define CORE_BASE_NAME "angrylion's RDP Plus"

#ifdef _DEBUG
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG " (Debug)"
#else
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG
#endif

#define CORE_SIMPLE_NAME "angrylion-plus"
