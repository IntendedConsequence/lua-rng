#include <lua.h>
#include <lauxlib.h>
#include <stdint.h>
#include "rng.h"	// TODO: rename


/*
** ===============================================================
**   Declarations
** ===============================================================
*/

// TODO: add declarations
typedef struct XoroshiroSeed
{
	uint64_t s0, s1;
} XoroshiroSeed;

/*
** ===============================================================
**   Generic internal code
** ===============================================================
*/

// TODO: add generic code

static __inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}

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

	// TODO: add functions from 'exposed Lua API' section above
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
	// TODO: add startup/initialization code
	lua_getglobal(L, "print");
	lua_pushstring(L, "Now initializing module 'required' as:");
	lua_pushvalue(L, 1); // pos 1 on the stack contains the module name
	lua_call(L, 2, 0);





	return 1;	// report success
}


// Close method called when Lua shutsdown the library
// Note: check Lua os.exit() function for exceptions,
// it will not always be called! Changed from Lua 5.1 to 5.2.
static int L_closeLib(lua_State *L) {


	// TODO: add shutdown/cleanup code
	lua_getglobal(L, "print");
	lua_pushstring(L, "Now closing the Lua template library");
	lua_call(L, 1, 0);




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
	return 1;
};

