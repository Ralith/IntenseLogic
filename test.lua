require "strict"

local script = require "script"
local drawable = require "drawable";
local context  = require "context";
local texture  = require "texture";
local material = require "material";
local camera   = require "camera";
local positionable = require "positionable"
local world = require "world"
local matrix = require "matrix"
local vector3 = require "vector3"
local mesh = require "mesh"
local event = require "event"
local input = require "input"
local quaternion = require "quaternion"
local light = require "light"

local w = world();
local c = context(800, 600);
c.world = w;
w.context = c.ptr;
c:setActive();
local m;
local t = texture()
t:fromfile("color0", "white-marble-texture.png")
local vf, ff = io.open("shaders/test.vert", "r"), io.open("shaders/test.frag", "r");
--local mtl = material(vf:read "*a", ff:read "*a", "test material", "in_Position", "in_Texcoord", "in_Normal", "mvp", {"tex"}, {1}, "out_Normal", "out_Ambient", "out_Diffuse", "out_Specular", "phong");
local mtl = material()
mtl:vertex(vf:read "*a")
mtl:fragment(ff:read "*a")
vf:close()
ff:close()
mtl:name "Test material"
mtl:arrayAttrib("position", "in_Position")
mtl:arrayAttrib("texcoord", "in_Texcoord")
mtl:arrayAttrib("normal", "in_Normal")
mtl:matrix("MVP", "mvp")
mtl:matrix("IM", "im")
mtl:textureUnit("color0", "tex")
mtl:fragData("normal", "out_Normal")
mtl:fragData("accumulation", "out_Ambient")
mtl:fragData("diffuse", "out_Diffuse")
mtl:fragData("specular", "out_Specular")
mtl:link()
if false then
    m = mesh "minecraft.obj"
    print(m.ptr)
    local minecraft = positionable()
    w:add(minecraft)
    minecraft.drawable = m;
    minecraft.material = mtl;
    minecraft.texture = texture.default;
    minecraft:track(c);
else
    m = mesh "teapot.obj"
    local width = 3
    for i = 0, width*width*width-1 do
        local box = positionable()
        w:add(box.ptr)
        box.drawable = m.ptr --drawable.box
        box.material = mtl.ptr
        box.texture = t.ptr
        box.position = (vector3(i % width, math.floor((i%(width*width)) / width), math.floor(i/(width*width))) * vector3(15, 15, 15)).ptr
        box:track(c.ptr)
        --print(box.position)
    end
end

c.camera = camera()
c.camera.projection_matrix = matrix.perspective(75, 4/3, 2, 1000).ptr
c.camera.positionable.position = vector3(0, -5, -25).ptr
c.camera.sensitivity = .01
c.camera.movespeed = vector3(1,1,1).ptr
--[[local l = light(-5, -5, -5, 50, 0, 0, 1.0) -- x y z radius r g b
print(l.positionable)
l:add(c)
local sig = positionable();
w:add(sig)
sig.position = vector3(-5, -5, -5)
sig.drawable = drawable.box;
sig.material = material.default;
sig.texture = texture.default;
sig:track(c)]]

local first_mouse = true
function mousemove(reg, name, x, y)
    if first_mouse then first_mouse = false return end
    if input.isButtonSet(0) == 0 then return end -- TODO: Make this work
    local yaw = quaternion(vector3(0, 1, 0), x * c.camera.sensitivity)
    local pitch = quaternion(vector3(1, 0, 0), y * c.camera.sensitivity)
    local rot = quaternion.wrap(c.camera.positionable.rotation) * yaw * pitch
    c.camera.positionable.rotation = rot.ptr
end

function tick(reg, name)
    --print "tick"
    local x = input.isKeySet("D") - input.isKeySet("A")
    local z = input.isKeySet("W") - input.isKeySet("S")
    local y = input.isKeySet("R") - input.isKeySet("F")
    local r = input.isKeySet("Q") - input.isKeySet("E")
    local v = vector3(x,y,z) * vector3.wrap(c.camera.movespeed)
    --print("r", c.camera.positionable.rotation)
    --print("v", v)
    v = v * quaternion.wrap(c.camera.positionable.rotation)
    --print("v'", v)
    c.camera.positionable.position = (vector3.wrap(c.camera.positionable.position) + v).ptr
    local bank = quaternion(vector3(0, 0, 1), r * c.camera.sensitivity)
    c.camera.positionable.rotation = (quaternion.wrap(c.camera.positionable.rotation) * bank).ptr
end

event.register(event.registry, "tick", tick)
event.register(event.registry, "input.mousemove", mousemove)

