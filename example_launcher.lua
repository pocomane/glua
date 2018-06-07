
--[[DOC

A more complex example to be embedded with linject, e.g. into luancher.exe.  It
is a lua script runner that handle some non-trivial cases.

luancher.exe will set the lua package.path and cpath so that lua will load all
the .lua and .dll files in the same directory (of luancher.exe). Then it will
try to execute two lua script.

The first script is config.lua in the same directory. Its purpose is to
configure global variables, so it is run in a sandbox environment just contain
- the variable 'this_directory' containing the obsolute path of the directory
    containing config.lua file with the links expanded
- the following standard lua globals: assert, coroutine, error, os.getenv,
    ipairs, math, next, pairs, pcall, string, table, tonumber, tostring, type,
    unpack, xpcall.
- the 'chainload' global function that will load another lua script in the same
    environment (it takes the path to the file as the argument)

If the config.lua file is not found luancher.exe will not raise any error.

The second script is the first match of the following list:
- the main.lua file in the same directory of the executable
- the .lua file with same name in the same directory of the executable (e.g.
    luancher.lua)
- the first argument ONLY IF it has the .lua extension
- the file handle_e_YYY.XXX.lua in the same directory of luancher.exe, where
    YYY is the laucher name (e.g. luancher) and .XXX is the extension of the
    file passed as first argument (can be empty) 
- the file handler_n_YYY.XXX.lua where YYY is the laucher name (e.g. luancher)
    and XXX is the first argument ONLY IF the first argument has no path and no
    extension

It is run in the full lua environment and if it is not found luancher will
print an error message containing all the script tryed.

Whatever script is run, it will receve the following arguments:
same arguments descibed in the
luaject section but with arg[0] replaced by 

- arg[-1] - command line path to the executable (the C argv[0]) with links resolved ( ??? TODO : check this !!! )
- arg[0] - the script that was chosen (absolute path with links resolved)
- arg[N] - (N>0) are the rest of command line arguments

Note that the directory containing the launcher, that is used for package.path
and cpath also, is the one passed by luamain.o as arg[-1] i.e. has all the
filesystem link resolved. So, for example, if you have

```
...
/bin/luancher.exe
/bin/luancher.lua
/bin/module.lua
/module.lua
/luancher.lua
/luancher.exe -> my_project/bin/luancher.exe
...
```

and /bin/luancher.lua contains 

```
local ex = require 'module'
```

while /luancher.lua is empty, launching /luancher.exe will
results in the loading of /bin/luancher.lua (not /luancher.lua) and then it will
require /bin/module.lua (not /module.lua)

Note that you can find tools similar to luancher around the web, e.g.
[l-bia](http://l-bia.sourceforge.net) (but I never tryed it).

Why all this words for such small tool? I changed a lot of times the way this
software work. So I just needed to FREEZE all, and print my thought somewhere
for the me of the future. :)

]]

  if type(arg[-1]) ~= 'string' then return nil end

  function path_split(str)
    local p = str:match('(.*[/\\])[^/\\]*')
    local n = str:match('([^/\\]*)$'):gsub('%.[^/\\%.]*$','')
    local e = str:match('%.[^/\\%.]*$')
    if not p then p = '' end
    if not n then n = '' end
    if not e then e = '' end
    return p,n,e
  end

  local realpath, progname, extension = path_split(whereami)
  package.path = realpath .. '?.lua;' .. realpath .. '?/init.lua'
  package.cpath = realpath .. '?.dll;' .. realpath .. 'lib?.so'

  local chainload = (function()
    local sandbox = {
      --print = print,
      assert = assert, coroutine = coroutine, error = error,
      os = {getenv = os.getenv}, ipairs = ipairs, math = math, next = next,
      pairs = pairs, pcall = pcall, string = string, table = table,
      tonumber = tonumber, tostring = tostring, type = type, unpack = unpack,
      xpcall = xpall,
    }
    sandbox.chainload = function (file)
      sandbox.this_directory = file:gsub('[/\\][^/\\]*$','')
      local t = io.open(file)
      if t then
        t:close()
        loadfile(file,'t',sandbox)()
      end -- missing config is not an error !
      return sandbox
    end
    return sandbox.chainload
  end)()

  local conf = chainload(realpath..'config.lua')
  -- CONF keys are merged into global when there is no conflict.
  -- Conflicting keys are avaiable in the CONFLICT global
  for k,v in pairs(conf) do
    if _G[k] == nil then
      _G[k] = v
    else
      if not CONFLICT then CONFLICT = {} end
      CONFLICT[k] = v
    end
  end

  local script_list = {}
  function script_list:append(s) script_list[1+#(script_list)]=s end

  script_list:append(realpath .. 'main.lua')
  script_list:append(realpath .. progname .. '.lua')
  if arg[1] then
    if ae == '.lua' then
      script_list:append (arg[1])
    end
    local ap,an,ae = path_split(arg[1])
    script_list:append (realpath .. 'handle_e_' .. progname .. ae .. '.lua')
    if ap == '' and ae == '' and an ~= '' then
      script_list:append (realpath .. 'handle_n_' .. progname .. '.' .. an .. '.lua')
    end
  end

  for _,s in ipairs(script_list) do
    local file = io.open(s,'rb')
    if file then
      file:close()
      arg[0] = s
      local chunk,err = loadfile(arg[0])
      if not chunk then
        error('error loading file '..arg[0]..':\n'..err)
      end
      return chunk()
    end
  end

  local err = ''
  if #(script_list) == 0 then
    err = ' None'
  else
    for _,s in ipairs(script_list) do
      err = err .. '\n  ' .. s
    end
  end
  error('Can not find the script to launch. Tryed: '..err)

