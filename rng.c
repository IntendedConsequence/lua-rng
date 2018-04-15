#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include "rng.h"


/*
** ===============================================================
**   Declarations
** ===============================================================
*/

typedef struct XoroshiroSeed
{
	uint64_t s0, s1;
} XoroshiroSeed;

/*
** ===============================================================
**   Generic internal code
** ===============================================================
*/

static __inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

/*  Written in 2015 by Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */
/* This is a fixed-increment version of Java 8's SplittableRandom generator
   See http://dx.doi.org/10.1145/2714064.2660195 and 
   http://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html

   It is a very fast generator passing BigCrush, and it can be useful if
   for some reason you absolutely want 64 bits of state; otherwise, we
   rather suggest to use a xoroshiro128+ (for moderately parallel
   computations) or xorshift1024* (for massively parallel computations)
   generator. */
/* The state can be seeded with any value. */
uint64_t splitmix(uint64_t x) {
	uint64_t z = (x += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

void setseed(XoroshiroSeed *seed, uint64_t newseed) {
	seed->s0 = splitmix(newseed);
	seed->s1 = splitmix(newseed);
}
/*  Written in 2016 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* This is the successor to xorshift128+. It is the fastest full-period
   generator passing BigCrush without systematic failures, but due to the
   relatively short period it is acceptable only for applications with a
   mild amount of parallelism; otherwise, use a xorshift1024* generator.

   Beside passing BigCrush, this generator passes the PractRand test suite
   up to (and included) 16TB, with the exception of binary rank tests, as
   the lowest bit of this generator is an LFSR of degree 128. The next bit
   can be described by an LFSR of degree 8256, but in the long run it will
   fail linearity tests, too. The other bits needs a much higher degree to
   be represented as LFSRs.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   Note that the generator uses a simulated rotate operation, which most C
   compilers will turn into a single instruction. In Java, you can use
   Long.rotateLeft(). In languages that do not make low-level rotation
   instructions accessible xorshift128+ could be faster.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. */
uint64_t next(XoroshiroSeed *seed) {
	const uint64_t s0 = seed->s0;
	uint64_t s1 = seed->s1;
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	seed->s0 = rotl(s0, 55) ^ s1 ^ (s1 << 14); // a, b
	seed->s1 = rotl(s1, 36); // c

	return result;
}

/*
** ===============================================================
**   Exposed Lua API
** ===============================================================
*/


int L_newseed(lua_State *L)
{
	size_t nbytes;
	XoroshiroSeed *seed;

	nbytes = sizeof(XoroshiroSeed);
	seed = (XoroshiroSeed *)lua_newuserdata(L, nbytes);
	seed->s0 = 0;
	seed->s1 = 0;
	luaL_getmetatable(L, "rng.XoroshiroSeed");
	lua_setmetatable(L, -2);
	return 1;	// number of return values on the Lua stack
};

XoroshiroSeed *checkseed(lua_State *L)
{
	void *ud;
	ud = luaL_checkudata(L, 1, "rng.XoroshiroSeed");
	luaL_argcheck(L, ud != NULL, 1, "seed expected");
	return (XoroshiroSeed *)ud;
}

int L_setseed(lua_State *L)
{
	XoroshiroSeed *seed;
	uint64_t newseed;
	seed = checkseed(L);
	newseed = luaL_checkinteger(L, 2);
	setseed(seed, newseed);
	return 0;
}

int L_next(lua_State *L)
{
	XoroshiroSeed *seed;
	seed = checkseed(L);
	lua_pushnumber(L, (next(seed) >> 11) * (1. / (UINT64_C(1) << 53)));
	return 1;
}


/*
** ===============================================================
** Library initialization and shutdown
** ===============================================================
*/

static const struct luaL_Reg LuaMetatableFunctions[] = {
	{"setseed", L_setseed},
	{"next", L_next},

	{NULL,NULL}  // last entry; list terminator
};

// Structure with all functions made available to Lua
static const struct luaL_Reg LuaExportFunctions[] = {
	{"new",L_newseed},

	{NULL,NULL}  // last entry; list terminator
};

// Open method called when Lua opens the library
// On success; return 1
// On error; push errorstring on stack and return 0
static int L_openLib(lua_State *L) {
	luaL_newmetatable(L, "rng.XoroshiroSeed");
	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2);
	lua_settable(L, -3);
	luaL_openlib(L, NULL, LuaMetatableFunctions, 0);
	return 1;	// report success
}


// Close method called when Lua shutsdown the library
// Note: check Lua os.exit() function for exceptions,
// it will not always be called! Changed from Lua 5.1 to 5.2.
static int L_closeLib(lua_State *L) {
	// TODO: add shutdown/cleanup code
	return 0;
}


/*
** ===============================================================
** Core startup functionality, no need to change anything below
** ===============================================================
*/


// Setup a userdata to provide a close method
static void L_setupClose(lua_State *L) {
	// create library tracking userdata
	if (lua_newuserdata(L, sizeof(void*)) == NULL) {
		luaL_error(L, "Cannot allocate userdata, out of memory?");
	};
	// Create a new metatable for the tracking userdata
	luaL_newmetatable(L, LTLIB_UDATAMT);
	// Add GC metamethod
	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, L_closeLib);
	lua_settable(L, -3);
	// Attach metatable to userdata
	lua_setmetatable(L, -2);
	// store userdata in registry
	lua_setfield(L, LUA_REGISTRYINDEX, LTLIB_UDATANAME);
}

// When initialization fails, removes the userdata and metatable
// again, throws the error stored by L_openLib(), will not return
// Top of stack must hold error string
static void L_openFailed(lua_State *L) {
	// get userdata and remove its metatable
	lua_getfield(L, LUA_REGISTRYINDEX, LTLIB_UDATANAME);
	lua_pushnil(L);
	lua_setmetatable(L, -2);
	// remove userdata
	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, LTLIB_UDATANAME);
	// throw the error (won't return)
	luaL_error(L, lua_tostring(L, -1));
}

LTLIB_EXPORTAPI	int LTLIB_OPENFUNC (lua_State *L){	

	// Setup a userdata with metatable to create a close method
	L_setupClose(L);

	if (L_openLib(L) == 0)  // call initialization code
		L_openFailed(L);    // Init failed, so cleanup, will not return

	// Export Lua API
	lua_newtable(L);
#if LUA_VERSION_NUM < 502
	luaL_register(L, LTLIB_GLOBALNAME, LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif
	lua_newtable(L);
	lua_pushstring(L, "__call");
	lua_pushcfunction(L, L_newseed);
	lua_settable(L, -3);
	lua_setmetatable(L, -2);
	return 1;
};


