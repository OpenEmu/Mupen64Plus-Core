#pragma once

#define GIT_BRANCH "master"
#define GIT_TAG "v2.5.3-34-dirty"
#define GIT_COMMIT_HASH "245113f4b5e4eac2b802f3d117c7cc22c9a3e33c"
#define GIT_COMMIT_DATE "2017-09-21 21:01:19 -0500"

#define CORE_BASE_NAME "angrylion's RDP Plus"

#ifdef _DEBUG
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG " (Debug)"
#else
#define CORE_NAME CORE_BASE_NAME " " GIT_TAG
#endif
