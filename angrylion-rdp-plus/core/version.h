#pragma once

#define GIT_BRANCH "master"
#define GIT_TAG "r4-5"
#define GIT_COMMIT_HASH "420a792ebbe91452b690fc691eb2e50a5c3c2456"
#define GIT_COMMIT_DATE "2017-09-28 12:50:20 +0200"

#define CORE_BASE_NAME "angrylion's RDP Plus"

#ifdef _DEBUG
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG " (Debug)"
#else
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG
#endif

#define CORE_SIMPLE_NAME "angrylion-plus"
