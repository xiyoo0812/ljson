#define LUA_LIB

#include "ljson.h"

namespace ljson {
    thread_local yyjson thread_json;
    static int encode(lua_State* L) {
        return thread_json.encode(L);
    }
    static int decode(lua_State* L) {
        size_t len;
        const char* buf = luaL_checklstring(L, 1, &len);
        return thread_json.decode(L, buf, len);
    }

    luakit::lua_table open_ljson(lua_State* L) {
        luakit::kit_state kit_state(L);
        auto ljson = kit_state.new_table();
        ljson.set_function("encode", encode);
        ljson.set_function("decode", decode);
        return ljson;
    }
}

extern "C" {
    LUALIB_API int luaopen_ljson(lua_State* L) {
        auto ljson = ljson::open_ljson(L);
        return ljson.push_stack();
    }
}