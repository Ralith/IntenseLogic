--- Provides wrappers around the internal script code
-- @type script
local ffi=require "ffi";

ffi.cdef[[

typedef struct il_string il_string;

il_string il_CtoS(const char * s, int len);

typedef struct ilA_asset ilA_asset;

typedef struct ilS_script ilS_script;

ilS_script * ilS_new();
int ilS_fromAsset(ilS_script*, ilA_asset * asset);
int ilS_fromSource(ilS_script*, il_string source);
int ilS_fromFile(ilS_script*, const char * filename);
int ilS_run(ilS_script*);

]]

local script = {}

--- Creates a new script
-- @treturn script The script
function script.create()
    local ptr = ffi.C.ilS_new();
    local obj = {};
    obj.ptr = ptr;
    setmetatable(obj, {__index = script});
    return obj;
end

--- Sets script source using an asset structure
-- @tparam asset asset The asset
function script:fromAsset(asset)
    assert(ffi.istype("struct ilA_asset*", asset.ptr), "Expected asset");
    if ffi.C.ilS_fromAsset(self.ptr, asset.ptr) == -1 then
        error("Failed to load script");
    end
end

--- Sets script source from a string
-- @tparam string s The source
function script:fromSource(s)
    assert(type(s) == "string", "Expected string");
    if ffi.C.ilS_fromSource(self.ptr, ffi.C.il_CtoS(s, -1)) == -1 then
        error("Failed to load script");
    end
end

--- Sets script source from a file
-- @tparam string name The file
function script:fromFile(name)
    assert(type(s) == "string", "Expected string");
    if ffi.C.ilS_fromFile(self, name) == -1 then
        error("Failed to load script");
    end
end

--- Runs a script
function script:run()
    if ffi.C.ilS_run(self) == -1 then
        error("Failed to run script");
    end
end

setmetatable(script, {__call = script.create})

return script;

