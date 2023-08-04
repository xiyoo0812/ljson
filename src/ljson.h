#pragma once

#include "yyjson.h"
#include "lua_kit.h"

using namespace std;
using namespace luakit;

const uint8_t max_encode_depth  = 16;

namespace ljson {
    class yyjson {
    public:
        int encode(lua_State* L) {
            size_t data_len;
            yyjson_write_err err;
            yyjson_mut_doc* doc = yyjson_mut_doc_new(nullptr);
            yyjson_mut_val* val = encode_one(L, doc, -1, 0);
            char* json = yyjson_mut_val_write_opts(val, YYJSON_WRITE_ALLOW_INVALID_UNICODE, nullptr, &data_len, &err);
            if (!json) luaL_error(L, err.msg);
            lua_pushlstring(L, json, data_len);
            yyjson_mut_doc_free(doc);
            free(json);
            return 1;
        }

        int decode(lua_State* L, const char* buf, size_t len) {
            yyjson_read_err err;
            yyjson_doc* doc = yyjson_read_opts((char*)buf, len, YYJSON_READ_ALLOW_INVALID_UNICODE, nullptr, &err);
            if (!doc) luaL_error(L, err.msg);
            decode_one(L, yyjson_doc_get_root(doc));
            yyjson_doc_free(doc);
            return 1;
        }

    protected:
        bool is_array(lua_State *L, size_t raw_len) {
            if (raw_len == 0) return false;
            lua_guard g(L);
            lua_pushnil(L);
            size_t cur_len = 0;
            while(lua_next(L, -2) != 0) {
                if (!lua_isinteger(L, -2)) {
                    return false;
                }
                cur_len++;
                lua_pop(L, 1);
            }
            return cur_len == raw_len;
        }

        yyjson_mut_val* encode_one(lua_State* L, yyjson_mut_doc* doc, int idx, int depth) {
            if (depth > max_encode_depth) {
                luaL_error(L, "encode can't pack too depth table");
            }
            int type = lua_type(L, idx);
            switch (type) {
            case LUA_TNIL:
                return yyjson_mut_null(doc);
            case LUA_TSTRING: {
                size_t len;
                const char* val = lua_tolstring(L, idx, &len);
                return yyjson_mut_strn(doc, val, len);
            }
            case LUA_TBOOLEAN:
                return yyjson_mut_bool(doc, lua_toboolean(L, idx));
            case LUA_TNUMBER:
                return lua_isinteger(L, idx) ? yyjson_mut_sint(doc, lua_tointeger(L, idx)) : yyjson_mut_real(doc, lua_tonumber(L, idx));
            case LUA_TTABLE:
                return table_encode(L, doc, idx, depth + 1);
            }
            return nullptr;
        }

        yyjson_mut_val* key_encode(lua_State* L, yyjson_mut_doc* doc, int idx) {
            switch (lua_type(L, idx)) {
            case LUA_TSTRING:
                return yyjson_mut_str(doc, lua_tostring(L, idx));
            case LUA_TNUMBER:
                if (lua_isinteger(L, idx)) {
                    return yyjson_mut_strcpy(doc, to_string(lua_tointeger(L, idx)).c_str());
                }
                return yyjson_mut_strcpy(doc, to_string(lua_tonumber(L, idx)).c_str());
            }
            return nullptr;
        }

        yyjson_mut_val* array_encode(lua_State* L, yyjson_mut_doc* doc, int index, int depth) {
            lua_pushnil(L);
            yyjson_mut_val* array = yyjson_mut_arr(doc);
            while (lua_next(L, index) != 0) {
                auto value = encode_one(L, doc, -1, depth);
                if (!value) {
                    luaL_error(L, "value is unsupported type");
                }
                yyjson_mut_arr_append(array, value);
                lua_pop(L, 1);
            }
            return array;
        }

        yyjson_mut_val* table_encode(lua_State* L, yyjson_mut_doc* doc, int index, int depth) {
            index = lua_absindex(L, index);
            if (!is_array(L, lua_rawlen(L, index))) {
                lua_pushnil(L);
                yyjson_mut_val* object = yyjson_mut_obj(doc);
                while (lua_next(L, index) != 0) {
                    auto key = key_encode(L, doc, -2);
                    auto value = encode_one(L, doc, -1, depth);
                    unsafe_yyjson_mut_obj_add(object, key, value, unsafe_yyjson_get_len(object));
                    lua_pop(L, 1);
                }
                return object;
            }
            return array_encode(L, doc, index, depth);
        }

        void number_decode(lua_State* L, yyjson_val *val) {
            switch (yyjson_get_subtype(val)) {
            case YYJSON_SUBTYPE_UINT:
            case YYJSON_SUBTYPE_SINT:
                lua_pushinteger(L, unsafe_yyjson_get_sint(val));
                break;
            case YYJSON_SUBTYPE_REAL:
                lua_pushnumber(L, unsafe_yyjson_get_real(val));
                break;
            }
        }

        void array_decode(lua_State* L, yyjson_val *val) {
            yyjson_arr_iter it;
            yyjson_arr_iter_init(val, &it);
            lua_createtable(L, 0, (int)yyjson_arr_size(val));
            while ((val = yyjson_arr_iter_next(&it))) {
                decode_one(L, val);
                lua_rawseti(L, -2, it.idx);
            }
        }

        void table_decode(lua_State* L, yyjson_val *val) {
            yyjson_obj_iter it;
            yyjson_val *key = nullptr;
            yyjson_obj_iter_init(val, &it);
            lua_createtable(L, 0, (int)yyjson_obj_size(val));
            while ((key = yyjson_obj_iter_next(&it))) {
                auto skey = unsafe_yyjson_get_str(key);
                if (lua_stringtonumber(L, skey) == 0) {
                    //默认尝试 string key 转 number
                    lua_pushlstring(L, skey, unsafe_yyjson_get_len(key));
                }
                decode_one(L, yyjson_obj_iter_get_val(key));
                lua_rawset(L, -3);
            }
        }

        void decode_one(lua_State* L, yyjson_val *val) {
            switch (yyjson_get_type(val)) {
            case YYJSON_TYPE_NULL:
            case YYJSON_TYPE_NONE:
                lua_pushnil(L);
                break;
            case YYJSON_TYPE_BOOL:
                lua_pushboolean(L, unsafe_yyjson_get_bool(val));
                break;
            case YYJSON_TYPE_STR:
                lua_pushlstring(L, unsafe_yyjson_get_str(val), unsafe_yyjson_get_len(val));
                break;
            case YYJSON_TYPE_NUM:
                number_decode(L, val);
                break;
            case YYJSON_TYPE_ARR:
                array_decode(L, val);
                break;
            case YYJSON_TYPE_OBJ:
                table_decode(L, val);
                break;
            default:
                lua_pushnil(L);
                break;
            }
        }
    };
}
