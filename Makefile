# 5.2, jit
LUAV?=5.2

LUA_CFLAGS 	= `pkg-config lua$(LUAV) --cflags`
LUA_LDFLAGS	= `pkg-config lua$(LUAV) --libs` -g -shared

CFLAGS		= -O2 -Wall -shared -fPIC -fexceptions $(LUA_CFLAGS) -I.
LDFLAGS		= -shared -g $(LUA_LDFLAGS)

CC 			= gcc
OUTPUT		= tnt.so

OBJS = src/tnt.o

all: $(OBJS)
	$(CC) -o $(OUTPUT) $(LDFLAGS) ${OBJS}
	cp -f tnt.so test/
	cp -f src/octopus.lua test/
	cp -f src/tnt_schema.lua test/
	cp -f src/tnt_helpers.lua test/

libs: yaml telescope pack

yaml:
	make -C 3rdparty/yaml all LUAV=$(LUAV)
	cp -f 3rdparty/yaml/yaml.so test/yaml.so

telescope:
	cp -f 3rdparty/telescope/* test/

pack:
	make -C 3rdparty/pack all LUAV=$(LUAV)
	cp -f 3rdparty/pack/tnt_pack.so test/tnt_pack.so

clean-all:
	make -C 3rdparty/yaml clean
	make -C 3rdparty/pack clean
	make clean

clean:
	rm -f test/*.so
	rm -f *.so
	rm -f $(OBJS)

docs:
	make -C docs html

test:
	 LUAV=$(LUAV) make -C test test

.PHONY: all libs yaml telescope pack clean-all clean docs test test_new
