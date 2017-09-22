#pragma once

#include "core.h"

void vi_init(struct core_config* config, struct plugin_api* plugin, struct screen_api* screen);
void vi_update(void);
void vi_screenshot(char* path);
void vi_close(void);
