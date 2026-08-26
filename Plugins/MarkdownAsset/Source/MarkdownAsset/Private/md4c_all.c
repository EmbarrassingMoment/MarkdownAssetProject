// Copyright (c) 2026 Kurorekishi (EmbarrassingMoment).
// Unity build file: compiles md4c sources from Source/ThirdParty/md4c/

// The md4c sources are third-party and must not be modified. Newer MSVC
// toolchains (UE 5.8+) report unreachable code (C4702) inside md4c.c and
// promote it to an error, so suppress it for the included sources here.
#ifdef _MSC_VER
#pragma warning(disable : 4702)
#endif

#include "md4c.c"
#include "md4c-html.c"
#include "entity.c"
