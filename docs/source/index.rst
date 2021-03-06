.. tarantool-lua documentation master file, created by
   sphinx-quickstart on Mon Nov 11 15:57:36 2013.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

=========================================
Welcome to tarantool-lua's documentation!
=========================================

Tarantool connector for Lua.

Contents:

.. toctree::
    :maxdepth: 2
    
    api

It Supports luajit, lua5.2 and lua5.1

Everything is simple:

.. code-block:: lua
    
    Connection = require("tarantool")
    def_schema = {
        spaces = {
            [0] = {
                fields = {'string', 'number32', 'number64'},
                indexes = {
                    [0] = {0},
                    [1] = {1},
                    [2] = {0, 1, 2},
                },
            },
            [1] = {
                ...
            },
        },
        funcs = {
            'box.time64' = {
                ['in'] = {},
                ['out'] = {'number64'}
            },
            'another stored procedur' = {
                ...
            },
        },
    } -- it's just an example. Schema may be smaller, simpler and more beautiful. 

    new_con = Connection{host = "127.0.0.1", port=33013, timeout=10, schema=def_schema}
    new_con:insert(0, {'hello', 123, 123456})
    new_con:insert(0, {'hello_1', 123, 1234567})
    new_con:select(0, 1, 'hello')

You may install this library with luarocks:

.. code-block:: bash

        # latest (unstable) version
        $ wget https://raw.github.com/bigbes92/tarantool-lua/master/rocks/tarantool-latest-1.rockspec
        # latest (stable) version
        $ wget https://raw.github.com/bigbes92/tarantool-lua/master/rocks/tarantool-0.0.1-1.rockspec
        $ luarocks install tarantool-{VERSION}.rockspec --local

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

