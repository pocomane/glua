
Glua
=====

This utility generates single binary file containing both the lua runtime and a
script to run.  You can compile it upon the standard lua or luajit, as well as
any C implementation of the lua API.

Any file without an explicit license reference is in the Public Domain. The
MIT-0 license can be alternatively applied. Please see the COPYING.txt file for
more informations.

Build
------

There is no actual build system. You can compile it with gcc using:

```
gcc -std=c99 -I . -o glua.exe *.c lua_lib -lm -ldl
```

This assumes that you have copied the lua headers in the current directoy and
the binary library (static or shared) in the lua_lib file. For windows you can
add -D_WIN32_WINNT=0x0600, and -mconsole or -mwindows (depending on if you want
or not a console to appear at start of the application).

// TODO : document gcc linker ORIGIN

glua.exe
---------

This app copies itself in target file, toghether with the script passed as
argument. When you execute the output, it will just execute the embedded script

The code that actually embed and extract the script is
[binject](kttps://github.com/pocomane/binject), so refer to it for option and
usage details. We just note that setting the define `BINJECT_ARRAY_SIZE` to
zero, you can force the script to be appended at end of the executable, so you
can edit the executable directly. 

The file `glued.exe` will be generate containing the script. When executed, the
script will be run with all the command line arguments. The defaults lua
globals will be avaiable, plus a `whereami` variable that contains the absolute
path to the executable.

If you want to embed lua VM code, use the default luac compiler and run glua
on the its output. As described in the lua manual, the lua bytecode is
compatible across machine with the same word size and byte order.

Please, be aware that glua.exe is (deliberately) an extremly simple tool. It
does not try to reduce size, or to embed other lua modules. For such advanced
operation you can use something like [lua
squish](http://matthewwild.co.uk/projects/squish/home) and then use glua.exe
on the resulting file.

There are other tools that archive somehow the same result of glua:
- [bin2c](https://sourceforge.net/p/wxlua/svn/217/tree/trunk/wxLua/util/bin2c/bin2c.lua)
    converts the lua bytestream in lua C API call ready to be compiled.
- [srlua](http://webserver2.tecgraf.puc-rio.br/~lhf/ftp/lua/#srlua) paste the
    script at end of the exe, and at run-time open the exe searching for the
    script.

Example
--------

If you have a hello_world.lua file containing
`print "hello world!"`, you can:

```
./glua.exe hello_world.lua
```

(or drag hello_world.lua on glua.exe). It will create hello_world.lua.exe
that contain the script. Launch it and the message `hello world!` will be
displayed in the console.

luancher
---------

The command

```
./glua.exe embed.lua
```

will generate int `glued.exe` a minimal lua script launcher. It simply load the
`init` file in its same directory, and run it as a lua script/bytecode. The
script will have access to the common lua globals, plus the `whereami` variable
previously described.

This tool is usefull while developing, when you are ready to deploy, you can
embed your script directly in a executable by means of `glua.exe`. In this
phase, if you have some issue with the package.path and cpath, just copy the
ones you found on the top of the default_launcher.lua (that is the script
embeded in luancher).

example_launcher
-----------------

A more complex lua script launcher is in the `example_laucher.lua`. Open it for
some documentation.

