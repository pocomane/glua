local INITFILE = 'init'
local f, err = io.open(INITFILE, 'rb')
if not f or err then
  io.stderr:write("Can not open " .. INITFILE .. " " .. err .. "\n")
  return -1
end
local s = f:read('a')
f:close()
local f, err = load(s)
if not f or err then
  io.stderr:write("Can not open " .. INITFILE .. " " .. err .. "\n")
  return -1
end
f()
