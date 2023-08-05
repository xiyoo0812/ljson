# ljson
lua bindings for yyjson(https://github.com/ibireme/yyjson).

# 依赖
- [lua](https://github.com/xiyoo0812/lua.git)5.3以上
- [luakit](https://github.com/xiyoo0812/luakit.git)一个luabind库
- [yyjson](https://github.com/ibireme/yyjson)，源码已经包含在内
- 项目路径如下<br>
  |--proj <br>
  &emsp;|--lua <br>
  &emsp;|--ljson <br>
  &emsp;|--luakit

# 接口说明
```lua
local ljson = require("ljson")
--编码
--value: 输入的lua
--pretty: 格式化输出json，取值（nil/0/1）
--res：输出字符串
--备注：会抛error，请使用pcall
local res = ljson.encode(value, pretty)

--解码
--value: 输入的字符串
--numkeyable: 将字符串key尝试转为数字key，取值（nil/0/1）
--res：输出lua
--备注：会抛error，请使用pcall
local res = ljson.decode(value, numkeyable)
```

# 用法参考
```lua
--本示例使用了quanta引擎
--https://github.com/xiyoo0812/quanta.git
local ljson         = require("ljson")
local cjson         = require("lcjson")

local log_debug     = logger.debug
local json_encode   = json.encode
local json_decode   = json.decode
local cjson_encode  = cjson.encode
local cjson_decode  = cjson.decode
local new_guid      = codec.guid_new
local ltime         = timer.time

local protobuf_mgr  = quanta.get("protobuf_mgr")

local test = {
    tid = 3.1415926,
    player_id = new_guid(),
    c = {[2]=1},
    d = {1, 2, 4, 5, 6},
    effect = {a=3, b=6}
}

local value = {
    error_code = 100005,
    user_id = 23024214324234,
    roles = {
        {role_id = 134234234, name = "asdasdas3", gender= 1, model = 231323},
        {role_id = 134234233, name = "asdasdas1", gender= 2, model = 231324},
        {role_id = 134234234, name = "asdasdas2", gender= 3, model = 231325},
    }
}

log_debug(test.tid)
log_debug(test.player_id)

local vv = {
    status = 0,
    timestamp = 0,
    proto_id = 40001,
    partners = {227942049067380831},
    detail = protobuf_mgr:encode_byname("ncmd_cs.login_account_login_res", value)
}

log_debug("%s, %s", #vv.detail, vv.detail)

local c = json_encode(vv, 1)
log_debug("json_encode2: %s", c)

local d = json_decode(c)
log_debug("json_decode: %s", d)

local dd = protobuf_mgr:decode_byname("ncmd_cs.login_account_login_res", d.detail)
log_debug("decode_byname: %s", dd)

local a = json_encode(test)
log_debug("json_encode: %s", a)

local b = json_decode(a, 1)
log_debug(type(b.tid), b.tid)
log_debug(type(b.player_id), b.player_id)
log_debug("json_decode: %s", b)

log_debug("%s", b.c)
log_debug("%s, %s", type(b.tid), b.tid)
log_debug("%s, %s", type(b.player_id), b.player_id)
for k, v in pairs(b.c) do
    log_debug("c %s, %s", type(k), k)
    log_debug("c %s, %s", type(v), v)
end

local encode_slice  = codec.encode_slice
local decode_slice  = codec.decode_slice
local tt = { region = 123, group = 3324, id = 122143556, name = "nodename", host = "127.0.0.1", port = 3369 }
local x1 = json_encode(tt)
log_debug("tt1:%s", x1)
local x2 = encode_slice(tt)
local ts = decode_slice(x2)
log_debug("tt21:%s", json_encode(ts))

local tt1 = ltime()
for i = 1, 200000 do
    json_encode(test)
end
local tt2 = ltime()
for i = 1, 200000 do
    cjson_encode(test)
end

local tt3 = ltime()
for i = 1, 200000 do
    json_decode(a)
end
local tt4 = ltime()
for i = 1, 200000 do
    cjson_decode(a)
end

local tt5 = ltime()
log_debug("tt1:%s, tt2:%s", tt2 - tt1, tt3 - tt2)
log_debug("tt3:%s, tt4:%s", tt4 - tt3, tt5 - tt4)

```
