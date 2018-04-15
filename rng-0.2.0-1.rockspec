package = "rng"
version = "0.2.0-1"

source = {
    url = "git://github.com/Tieske/Lua_library_template.git",
}
description = {
   summary = "xoroshiro128+ rng for lua",
   detailed = [[
      A basic implementation of xoroshiro128+ prng algorithm
      packaged as a Lua module, using splitmix64 to set the seed.
      Allows creating independent generators with private seeds.
   ]],
   homepage = "https://github.com/Tieske/Lua_library_template",
   license = "MIT"
}
dependencies = {
   "lua >= 5.1, < 5.4"
}
build = {
  type = "builtin",
  modules = {
    ["rng"] = {
      sources = {
        "rng.c",
      },
    },
  },
}
