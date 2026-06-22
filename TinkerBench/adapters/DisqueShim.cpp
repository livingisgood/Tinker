// DisqueShim.cpp
//
// The disque skiplist.c we vendored uses `#define zmalloc malloc` /
// `#define zfree free` directly (see the top of skiplist.c), so no zmalloc
// symbol needs to be linked from here. What this TU does provide is a
// one-time seed for the C rand() that disque's skiplistRandomLevel() uses,
// so the level distribution is non-degenerate across runs.

#include <cstdlib>
#include <ctime>

namespace {

struct DisqueRandSeeder
{
    DisqueRandSeeder() { std::srand(static_cast<unsigned>(std::time(nullptr))); }
} g_DisqueRandSeeder;

} // namespace
