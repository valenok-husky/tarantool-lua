package = "octopus"
version = "0.0.3-1"

source = {
    url = "git://github.com/valenok-husky/tarantool-lua.git",
    tag = "v0.0.3"
}

description = {
    summary = "A library to work with Tarantool In-Memory DB",
    detailed = [[
        The library currently allows user to INSERT, DELETE, SELECT, UPDATE
        and CALL stored procedures for Tarantool.
    ]],
    maintainer = "Nikita Galushko",
    license = "MIT",
}

dependencies = {
    "lua >= 5.1"
}

build = {
    type = "builtin",
    modules = {
        ["tnt"] = {
            sources = {
                "src/tnt.c"
            },
            incdirs = {
                "include/",
                "./"
            }
        },
        ["octopus"] = 'src/octopus.lua',
        ["tnt_schema"] = 'src/tnt_schema.lua',
        ["tnt_helpers"] = 'src/tnt_helpers.lua',
        ["tnt_pack"] = {
            sources = {
                "3rdparty/pack/pack.c"
            },
        },
    },
}
