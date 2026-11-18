/////////////////////////////////////////////////////////////////
// SoftX - Software Graphics API
// Copyright (c) 2026 NSDeathman
// Licensed under the MIT License.
/////////////////////////////////////////////////////////////////
#pragma once
/////////////////////////////////////////////////////////////////
// Math lib
#include <AfterMath.h>
/////////////////////////////////////////////////////////////////
// Your custom profiler
#ifdef ENABLE_PROFILER
#include "../../Third-Party/Optick/include/optick.h"
#define PROFILE_FRAME(x) OPTICK_FRAME(x)
#define PROFILE_SCOPE(x) OPTICK_EVENT(x)
#define PROFILE_THREAD(x) OPTICK_THREAD(x)
#define PROFILE_TAG(NAME, STATE) OPTICK_TAG(NAME, STATE)
#define PROFILE_START_CAPTURE() OPTICK_START_CAPTURE()
#define PROFILE_STOP_CAPTURE() OPTICK_STOP_CAPTURE()
#define PROFILE_SAVE_CAPTURE(name) OPTICK_SAVE_CAPTURE(name)
#else
#define PROFILE_FRAME(x)
#define PROFILE_SCOPE(x)
#define PROFILE_THREAD(x)
#define PROFILE_TAG(NAME, STATE)
#define PROFILE_START_CAPTURE()
#define PROFILE_STOP_CAPTURE()
#define PROFILE_SAVE_CAPTURE(name)
#endif
/////////////////////////////////////////////////////////////////
#define SOFTX_OPEN_NAMESPACES using namespace AfterMath;
/////////////////////////////////////////////////////////////////
