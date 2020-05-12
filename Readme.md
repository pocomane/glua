
Glua
=====

This utility generates single binary file containing both the lua runtime and a
script to run.  You can compile it upon the standard lua or luajit, as well as
any C implementation of the lua API.

This software is released under the [Unlicense](http://unlicense.org), so the
legal statement in COPYING.txt applies, unless differently stated in individual
files.

Build
------

There is no actual build system. You can compile it with gcc using:

```
gcc -DBINJECT_ARRAY_SIZE=1 -DUSE_WHEREAMI -I . -o glua.exe *.c lua_lib -lm -ldl
```

This assumes that you have copied the lua headers in the current directoy and
the binary library (static or shared) in the `lua_lib file`. For windows you can
add `-D_WIN32_WINNT=0x0600`, and `-mconsole` or `-mwindows` (depending on if
you want or not a console to appear at start of the application).

// TODO : document gcc linker ORIGIN

glua.exe
---------

This app copies itself in target file, toghether with the script passed as
argument. When you execute the output, it will just execute the embedded script

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

As example:

```
echo "print'hello world!" > hello_world.lua
./glua.exe hello_world.lua
```

(or drag `hello_world.lua` on `glua.exe`). It will create `glued.exe`
that contain the script. Launch it and the message `hello world!` will be
displayed in the console.

Build options
--------------

If the compilation flag `USE_WHEREAMI` is enabled, the `whereami` will use some
system dependent code to guess where the binary is. Otherwise it will use the
first command line argument.

If you define `ENABLE_STANDARD_LUA_CLI` pointing the lua `lua.c`, it will be
included in the `glua.exe`/`glued.exe` binaries. This enable to run the
standard lua interpreter when `--lua` is passed as the LAST argument to
`glua.exe` or `glued.exe`. Please note that the macro definition must begin and
end `"`, e.g.  `gcc -DENABLE_STANDARD_LUA_CLI='"/path/tp/lua.c"' ...`

The code that actually embed and extract the script is [binject](#Binject), so
refer to its [documentation](#Binject working) for additional options.

Link extra modules
-------------------

To embed extra C modules in `glua.exe`, just call `luaL-openlibs`-like function
fron `preload.c`.

// TODO : multiple script from command line -> include wrapping in a
require-able enclosure

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


Binject
--------

The code that actually embed the script in the executable is in the
`binject.c/h` files. It is designed as a "Library" so you can use them alone
without lua or anything else. You just need to provide the parsing function or
manipulate the data before the injection.

A complete but simple "Echo" example is provided for reference. It can be
compiled with

```
gcc -std=c99 -o binject.exe binject.c binject_example.cc
```

As it is, this will compile the "Echo" example. For some customization, read
the 'How it works' section.

When called without argument, some help information will be printed. To embed a
script pass it as argument.

```
./binject.exe my_script
```

This will generate the file injed.exe. Then:

```
echo "hello world" > my_text.txt
./binject.exe my_text.txt
rm my_text.txt
./injed.exe
```

will print "hello world" to the screen.

In the test directory there is a test that will execute all the commands seen
before. At end it will also check that the output of the example app is the
expected one.

Binject working
----------------

Two methods are avaiable to embed the script. By default, the "Array" method
will be tryed first, and if the script is too big, it will fallback to the
"Tail" method.

In the "Array" method the script will overwrite the initialization data of a
static struct.

In the "Tail" method the script will be appended at end of the
executable, and in the static struct will be kept only the informations
about where the script begin. With this method you can edit you script
directly in the exectuable.

The example application can be configured at compile time by means of the
following definitions.

`BINJECT_ARRAY_SIZE` - Size of the data for the INTERNAL ARRAY
mechanism. It should be a positive integer. If you put this value to 0,
you can actually force to always use the tail method. The default is
9216 byte.

