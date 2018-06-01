Magnum Neovim API
=================

*Neovim API bindings generator and library using [Magnum](http://magnum.graphics) types*

A small generated library which talks to [neovim](https://neovim.io) via TCP and their
[msgpack-rpc](https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md)
[API](https://github.com/neovim/neovim/blob/master/runtime/doc/msgpack_rpc.txt).

Note: Only Windows is supported currently. Except if you swiftly implement a
[Corrade::Net::Socket](https://github.com/Squareys/magnum-neovim-api/blob/master/src/Corrade/Net/Socket.cpp)
for your operating system (and hopefully create a pullrequest that).

# Building

The project relies on [CMake](https://cmake.org/). You can use
[vcpkg](https://github.com/Microsoft/vcpkg) or
[build Corrade and Magnum yourself](http://doc.magnum.graphics/magnum/building.html).
After that, building this project should be a matter of:

~~~
mkdir build && cd build
cmake .. -DBUILD_TESTS=ON
cmake --build . --target install
~~~

Potentially with additional parameters like `CMAKE_INSTALL_PREFIX` or
`CMAKE_TOOLCHAIN_FILE` depending on your specific setup.

# Running

Make sure to run Neovim first. While the documentation still refers to `neovim --listen 127.0.0.1:6666`,
`--listen` seems to be an unknown argument.
I found that instead the following works:

~~~
./nvim.exe --headless -c "call serverstart('127.0.0.1:6666')"
~~~

You can then run a function like this:

~~~cpp
    #include "neovimapi2.h"

    // ...

    NeovimApi::NeovimApi2 nvim{6666};
    Object result = nvim.nvim_eval("'hello' . 'world!\n'");
~~~

or just run the test `src/Test/NeovimApiTest.cpp`.

# How it works

## Code Generation

A [python3](https://python.org) script runs `nvim --api-info` to get a list of functions
and some metadata in *msgpack* format. This data is parsed and passed onto a
[jinja](http://jinja.pocoo.org) template which generates `neovimapi2.h` and `neovimapi2.cpp`
into `<build>/src/.`.

## CMake

`neovimapi2.h` and `neovimapi2.cpp` are generated via a
[custom CMake target](https://cmake.org/cmake/help/v3.0/command/add_custom_target.html) `generate`,
which allows the compilation of those files to depend on the code generation.
While this made working with the templates extremely convenient, it is not necessary to generate the
files more than once during development.
But, hey, now you have to checkout 1864 (.cpp) + 276 (.h) less lines of code `¯\_(ツ)_/¯`

## Communication with Neovim

Neovim allows two ways of communiaction: TCP and stdin. Data passed between Neovim and your client
is formatted in *msgpack*, which is kinda like json, but efficiently packed and binary.

On top of *msgpack*, [msgpack-rpc](https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md)
defines a protocol for remote procedure calls, which Neovim uses to allow you to run functions.

# Licence

The code of this project is licensed under the MIT/Expat license:

~~~
Copyright © 2018 Jonathan Hale <squareys@googlemail.com>

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
~~~
