#pragma once
#ifdef __TINKER_ASSERT
#define TK_ASSERT(exp) __TINKER_ASSERT(exp)
#else
#include <cassert>
#define TK_ASSERT(exp) assert(exp)
#endif