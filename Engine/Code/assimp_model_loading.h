#pragma once

#include "platform.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif // !_CRT_SECURE_NO_WARNINGS

struct App;

u32 LoadModel(App* app, const char* filename);
