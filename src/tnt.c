/*
 * lua_tarantool.c
 * Bindings for tp.h
 */

#if defined (__cplusplus)
extern "C" {
#endif

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#if defined (__cplusplus)
}
#endif

#if !defined(LUA_VERSION_NUM) || LUA_VERSION_NUM == 501
#	define luaL_newlib(L, num) (lua_newtable((L)),luaL_setfuncs((L), (num), 0))

	void luaL_setfuncs (lua_State *L, const luaL_Reg *l, int nup) {
		luaL_checkstack(L, nup+1, "too many upvalues");
		for (; l->name != NULL; l++) {  /* fill the table with given functions */
			int i;
			lua_pushstring(L, l->name);
			for (i = 0; i < nup; i++)  /* copy upvalues to the top */
				lua_pushvalue(L, -(nup + 1));
			lua_pushcclosure(L, l->func, nup);  /* closure with those upvalues */
			lua_settable(L, -(nup + 3)); /* table must be below the upvalues, the name and the closure */
		}
		lua_pop(L, nup);  /* remove upvalues */
	}
#endif /* !defined(LUA_VERSION_NUMBER) || LUA_VERSION_NUMBER == 501 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#include <3rdparty/tp/tp.h>

typedef struct tnt_Enum {
	const char *name;
	const int val;
} luatarantool_Enum;

typedef enum {
	TNT_OP_SET = 0,
	TNT_OP_ADD,
	TNT_OP_AND,
	TNT_OP_XOR,
	TNT_OP_OR,
	TNT_OP_SPLICE,
	TNT_OP_DELETE,
	TNT_OP_INSERT,
} tnt_requestbuilder_op;

typedef enum {
	TNT_BOX_RETURN_TUPLE = 0x01,
	TNT_BOX_ADD = 0x02,
	TNT_BOX_REPLACE = 0x04,
} tnt_requetbuilder_flag;

void lregister_enum(struct lua_State *L, int narg,
			const struct tnt_Enum *e,
			const char *str) {
	if (narg < 0)
		narg = lua_gettop(L) + narg + 1;
	lua_createtable(L, 0, 10);
	for (; e->name; ++e) {
		luaL_checkstack(L, 1, "enum too large");
		lua_pushinteger(L, e->val);
		lua_setfield(L, -2, e->name);
	}
	lua_setfield(L, -2, str);
}

inline const char *
ltnt_checkstring(struct lua_State *L, int narg, size_t *len) {
	if (!lua_isstring(L, narg))
		luaL_error(L, "Incorrect method call (expected string, got %s)",
				lua_typename(L, lua_type(L, narg)));
	return lua_tolstring(L, narg, len);
}

inline struct tp **
ltnt_checkresponseparser(struct lua_State *L, int narg) {
	return (struct tp **) luaL_checkudata(L, narg, "ResponseParser");
}
inline struct tp **
ltnt_checkrequestbuilder(struct lua_State *L, int narg) {
	return (struct tp **) luaL_checkudata(L, narg, "RequestBuilder");
}

inline int
ltnt_getindex(struct lua_State *L, int narg, int pos) {
	if (narg < 0)
		narg = lua_gettop(L) + narg + 1;
	lua_pushnumber(L, pos);
	lua_gettable(L, narg);
	return 0;
}

int ltnt_pushtuple(struct lua_State *L, struct tp **iproto, int narg) {
	if (narg < 0)
		narg = lua_gettop(L) + narg + 1;
	lua_pushnil(L);
	while(lua_next(L, narg) != 0) {
		size_t len = 0;
		const void *str = ltnt_checkstring(L, -1, &len);
		if (tp_field(*iproto, (const void *)str, len) == -1)
			return -1;
		lua_pop(L, 1);
	}
	return 0;
}

inline void ltnt_pushsntable(lua_State *L, int narg, const char *str, int num) {
	if (narg < 0)
		narg = lua_gettop(L) + narg + 1;
	lua_pushstring(L, str);
	lua_pushnumber(L, num);
	lua_settable(L, narg);
}


/*
 * Creating of PING request.
 * Must be called with:
 * reqid: LUA_TNUMBER
 *
 * returns LUA_TSTRING with binary packed request
 */
int ltnt_requestbuilder_ping(struct lua_State *L) {
	if (lua_gettop(L) != 2)
		luaL_error(L, "bad number of arguments (1 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid = (uint32_t )luaL_checkint(L, 2);
	if (tp_ping(*iproto) == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h memory error");
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/*
 * Creating of INSERT request.
 * Must be called with:
 * reqid: LUA_TNUMBER
 * space: LUA_TNUMBER
 * flags: LUA_TNUMBER
 * table: LUA_TTABLE as folowing:
 * { val_1, val_2, ... }
 * where val_N is converted to binary
 * string representation of value
 *
 * returns LUA_TSTRING with binary packed request
*/
int ltnt_requestbuilder_insert(struct lua_State *L) {
	if (lua_gettop(L) != 5)
		luaL_error(L, "bad number of arguments (4 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid = (uint32_t )luaL_checkint(L, 2);
	uint32_t space = (uint32_t )luaL_checkint(L, 3);
	uint32_t flags = (uint32_t )luaL_checknumber(L, 4);
	if (!lua_istable(L, 5))
		luaL_error(L, "Bad argument #4: (table expected, got %s)",
				lua_typename(L, lua_type(L, 5)));

	if (tp_insert(iproto[0], space, flags) == -1 ||
	    tp_tuple(*iproto) == -1 ||
	    ltnt_pushtuple(L, iproto, 5) == -1 ) {
		lua_pushstring(L, "tp.h memory error");
		lua_pushboolean(L, 0);
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/*
 * Creating of SELECT request.
 * Must be called with:
 * reqid : LUA_TNUMBER
 * space : LUA_TNUMBER
 * index : LUA_TNUMBER
 * offset: LUA_TNUMBER
 * limit : LUA_TNUMBER
 * table : LUA_TTABLE as following:
 * { 0 : {key1_p1, key1_p2, ...},
 *   1 : {key2_p1, ...},
 *   ...
 *   N : {keyN_p1, ...}
 * }
 * where keyN_pM is converted to binary
 * string representation of value
 *
 * returns LUA_TSTRING with binary packed request
 */
int ltnt_requestbuilder_select(struct lua_State *L) {
	if (lua_gettop(L) != 7)
		luaL_error(L, "bad number of arguments (6 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid  = (uint32_t )luaL_checkint(L, 2);
	uint32_t space  = (uint32_t )luaL_checkint(L, 3);
	uint32_t index  = (uint32_t )luaL_checkint(L, 4);
	uint32_t offset = (uint32_t )luaL_checkint(L, 5);
	uint32_t limit  = (uint32_t )luaL_checkint(L, 6);
	if (!lua_istable(L, 7))
		luaL_error(L, "Bad argument #6: (table expected, got %s)",
				lua_typename(L, lua_type(L, 7)));

	if (tp_select(*iproto, space, index, offset, limit) == -1){
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h memory error");
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushnil(L);
	while (lua_next(L, 7) != 0) {
		if (!lua_istable(L, -1))
			luaL_error(L, "Bad table construction: (table expected, got %s)",
					lua_typename(L, lua_type(L, -1)));
		if (tp_tuple(*iproto) == -1 ||
		    ltnt_pushtuple(L, iproto, -1) == -1) {
			lua_pushboolean(L, 0);
			lua_pushstring(L, "tp.h memory error");
			return 2;
		}
		lua_pop(L, 1);
	}
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/*
 * Creating of DELETE request.
 * Must be called with:
 * reqid : LUA_TNUMBER
 * space : LUA_TNUMBER
 * flags : LUA_TNUMBER
 * tuple : LUA_TTABLE as following:
 * { key_p1, key_p2, ... }
 * where key_pM is converted to binary
 * string representation of value
 *
 * returns LUA_TSTRING with binary packed request
 */
int ltnt_requestbuilder_delete(struct lua_State *L) {
	if (lua_gettop(L) != 5)
		luaL_error(L, "bad number of arguments (4 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid = (uint32_t )luaL_checkint(L, 2);
	uint32_t space = (uint32_t )luaL_checkint(L, 3);
	uint32_t flags = (uint32_t )luaL_checknumber(L, 4);
	if (!lua_istable(L, 5))
		luaL_error(L, "Bad argument #4: (table expected, got %s)",
				lua_typename(L, lua_type(L, 5)));
	if (tp_delete(*iproto, space, flags) == -1 ||
	    tp_tuple(*iproto) == -1 ||
	    ltnt_pushtuple(L, iproto, 5) == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h memory error");
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/*
 * Creating of CALL request.
 * Must be called with:
 * reqid : LUA_TNUMBER
 * name  : LUA_TSTRING
 * tuple : LUA_TTABLE as following:
 * { arg_1, arg_2, ... }
 * where arg_M is converted to binary
 * string representation of value
 */
int ltnt_requestbuilder_call(struct lua_State *L) {
	if (lua_gettop(L) != 4)
		luaL_error(L, "bad number of arguments (3 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid = (uint32_t )luaL_checkint(L, 2);
	size_t name_size = 0;
	const char *name = ltnt_checkstring(L, 3, &name_size);
	if (!lua_istable(L, 4))
		luaL_error(L, "Bad argument #3: (table expected, got %s)",
				lua_typename(L, lua_type(L, 4)));

	if (tp_call(*iproto, 0, name, name_size) == -1||
	    tp_tuple(*iproto) == -1 ||
	    ltnt_pushtuple(L, iproto, 4) == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h memory error");
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/*
 * Creating of UPDATE request.
 * Must be called with:
 * reqid: LUA_TNUMBER
 * space: LUA_TNUMBER
 * flags: LUA_TNUMBER
 * table: LUA_TTABLE as folowing:
 * { val_1, val_2, ... }
 * where val_N is converted to binary
 * string representation of value
 * ops  : LUA_TTABLE as following:
 * { 0 : {OP, field, data}
 *   1 : {SPLICE, field, offset, cut, data}
 *   ...
 * }
 *
 * returns LUA_TSTRING with binary packed request
*/
int ltnt_requestbuilder_update(struct lua_State *L) {
	if (lua_gettop(L) != 6)
		luaL_error(L, "bad number of arguments (5 expected, got %d)",
				lua_gettop(L) - 1);
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	uint32_t reqid = (uint32_t )luaL_checkint(L, 2);
	uint32_t space = (uint32_t )luaL_checkint(L, 3);
	uint32_t flags = (uint32_t )luaL_checknumber(L, 4);
	if (!lua_istable(L, 5))
		luaL_error(L, "Bad argument #4: (table expected, got %s)",
				lua_typename(L, lua_type(L, 5)));
	if (!lua_istable(L, 6))
		luaL_error(L, "Bad argument #5: (table expected, got %s)",
				lua_typename(L, lua_type(L, 6)));
	int ops = 6;
	int opcur = 8;
	if (tp_update(*iproto, space, flags) == -1 ||
	    tp_tuple(*iproto) == -1 ||
	    ltnt_pushtuple(L, iproto, 5) == -1 ||
	    tp_updatebegin(*iproto) == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h memory error");
		return 2;
	}
	tp_reqid(*iproto, reqid);
	lua_pushnil(L);
	while (lua_next(L, ops) != 0) {
		if (!lua_istable(L, opcur))
			luaL_error(L, "Bad table construction: (table expected, got %s)",
					lua_typename(L, lua_type(L, -1)));
		ltnt_getindex(L, opcur, 1);
		uint8_t op = (uint8_t )luaL_checkint(L, -1);
		lua_pop(L, 1);
		ltnt_getindex(L, opcur, 2);
		uint32_t field = (uint32_t )luaL_checkint(L, -1);
		lua_pop(L, 1);
		ltnt_getindex(L, opcur, 3);
		if (op == TNT_OP_SPLICE) {
			uint32_t offset = (uint32_t )luaL_checkint(L, -1);
			lua_pop(L, 1);
			ltnt_getindex(L, opcur, 4);
			uint32_t cut = (uint32_t )luaL_checkint(L, -1);
			lua_pop(L, 1);
			ltnt_getindex(L, opcur, 5);
			size_t len = 0;
			const char *data = ltnt_checkstring(L, -1, &len);
			if (tp_opsplice(*iproto, field, offset,
					cut, data, len) == -1) {
				lua_pushboolean(L, 0);
				lua_pushstring(L, "tp.h memory error");
				return 2;
			}
		}
		else {
			const char *data = NULL;
			size_t len = 0;
			if (lua_isstring(L, -1)) {
				data = ltnt_checkstring(L, -1, &len);
			} else if (lua_isnumber(L, -1)) {
				data = (char *)(intptr_t )luaL_checkint(L, -1);
				len = 4;
			} else {
				luaL_error(L, "Bad op argument: (string or number expected, got %s)",
					lua_typename(L, lua_type(L, -1)));
			}
			if (tp_op(*iproto, field, op, data, len) == -1) {
				lua_pushboolean(L, 0);
				lua_pushstring(L, "tp.h memory error");
				return 2;
			}
		}
		lua_pop(L, 2);
	}
	lua_pushboolean(L, 1);
	lua_pushnil(L);
	return 2;
}

/* Get request for RequestBuilder `class` */
int ltnt_requestbuilder_getval(lua_State *L) {
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	lua_pushlstring(L, tp_buf(*iproto), tp_used(*iproto));
	return 1;
}

/* Clear buffer for RequestBuilder `class` */
int ltnt_requestbuilder_flush(lua_State *L) {
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	tp_init(*iproto, tp_buf(*iproto), tp_size(*iproto), tp_realloc, NULL);
	return 0;
}

/* GC method for RequestBuilder `class` */
int ltnt_requestbuilder_gc(lua_State *L) {
	struct tp **iproto = ltnt_checkrequestbuilder(L, 1);
	tp_free(*iproto);
	free(*iproto);
	return 0;
}


int ltnt_requestbuilder_new(lua_State *L) {
	struct tp **iproto = lua_newuserdata(L, sizeof(struct tp*));
	*iproto = (struct tp *)malloc(sizeof(struct tp));
	tp_init(*iproto, NULL, 0, tp_realloc, NULL);
	luaL_getmetatable(L, "RequestBuilder");
	lua_setmetatable(L, -2);
	return 1;
}


int ltnt_response_bodylen(struct lua_State *L) {
	size_t resps = 0;
	char *resp = (char *)ltnt_checkstring(L, 1, &resps);
	lua_pushnumber(L, tp_reqbuf(resp, 12));
	return 1;
}

int ltnt_responseparser_parse(struct lua_State *L) {
	lua_checkstack(L, 10);
	struct tp **iproto = ltnt_checkresponseparser(L, 1);
	size_t resps = 0;
	char *resp = (char *)ltnt_checkstring(L, 2, &resps);
	/* Check HEADER_LEN */
	if (resps < 12)
		luaL_error(L, "ResponseParser: expected at least"
				" 12 bytes, got %d", resps);
	tp_init(*iproto, resp, 12, NULL, NULL);
	ssize_t required = tp_reqbuf(resp, 12);
	if (required + 12 != resps)
		luaL_error(L, "ResponseParser: expected"
				" %d bytes, got %d", resps+required, resps);
	tp_init(*iproto, resp, resps, NULL, NULL);
	ssize_t  sc  = tp_reply(*iproto);
	if (sc == -1) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "tp.h bad answer");
		return 2;
	}
	ssize_t ec  = sc >> 8;
	sc = sc & 0xFF;
	lua_pushboolean(L, 1);
	lua_createtable(L, 0, 5);
	ltnt_pushsntable(L, -1, "reply_code", sc);
	ltnt_pushsntable(L, -1, "operation_code", tp_replyop(*iproto));
	ltnt_pushsntable(L, -1, "tuple_count", tp_replycount(*iproto));
	ltnt_pushsntable(L, -1, "request_id", tp_getreqid(*iproto));

	if (sc != 0) {
		lua_pushstring(L, "error");
		lua_createtable(L, 0, 2);
		ltnt_pushsntable(L, -1, "errcode", ec);
		lua_pushstring(L, "errstr");
		lua_pushlstring(L, tp_replyerror(*iproto),
				tp_replyerrorlen(*iproto));
		lua_settable(L, -3);
		lua_settable(L, -3);
	} else {
		lua_pushstring(L, "tuples");
		lua_createtable(L, 0, tp_replycount(*iproto));
		ssize_t tup_num = 1;
		while (tp_next(*iproto)) {
			lua_pushnumber(L, tup_num);
			lua_createtable(L, tp_tuplecount(*iproto), 0);
			ssize_t fld_num = 1;
			while(tp_nextfield(*iproto)) {
				lua_pushnumber(L, fld_num);
				lua_pushlstring(L, tp_getfield(*iproto),
						tp_getfieldsize(*iproto));
				lua_settable(L, -3);
				fld_num++;
			}
			lua_settable(L, -3);
			tup_num++;
		}
		lua_settable(L, -3);
	}
	return 2;
}

/* GC method for ResponseParser `class` */
int ltnt_responseparser_gc(struct lua_State *L) {
	struct tp **iproto = ltnt_checkresponseparser(L, 1);
	free(*iproto);
	return 0;
}

int ltnt_responseparser_new(struct lua_State *L) {
	struct tp **iproto = lua_newuserdata(L, sizeof(struct tp*));
	*iproto = (struct tp *)malloc(sizeof(struct tp));
	luaL_getmetatable(L, "ResponseParser");
	lua_setmetatable(L, -2);
	return 1;
}

static const struct tnt_Enum ops[] = {
	{ "OP_SET"	,TNT_OP_SET		},
	{ "OP_ADD"	,TNT_OP_ADD		},
	{ "OP_AND"	,TNT_OP_AND		},
	{ "OP_XOR"	,TNT_OP_XOR		},
	{ "OP_OR"	,TNT_OP_OR		},
	{ "OP_SPLICE"	,TNT_OP_SPLICE		},
	{ "OP_DELETE"	,TNT_OP_DELETE		},
	{ "OP_INSERT"	,TNT_OP_INSERT		},
	{ NULL		,0			}
};

static const struct tnt_Enum flags[] = {
	{ "RETURN_TUPLE",TNT_BOX_RETURN_TUPLE	},
	{ "BOX_ADD"	,TNT_BOX_ADD		},
	{ "BOX_REPLACE"	,TNT_BOX_REPLACE	},
	{ NULL		,0			}
};


static const struct luaL_Reg lrequestresponse[] = {
	{ "request_builder_new"	,ltnt_requestbuilder_new},
	{ "response_parser_new"	,ltnt_responseparser_new},
	{ "get_body_len"	,ltnt_response_bodylen	},
	{ NULL			,NULL			}
};

static const struct luaL_Reg lresponseparser_meta[] = {
	{ "parse"	,ltnt_responseparser_parse	},
	{ "__gc"	,ltnt_responseparser_gc		},
	{ NULL		,NULL				}
};

static const struct luaL_Reg lrequestbuilder_meta[] = {
	{ "ping"	,ltnt_requestbuilder_ping	},
	{ "insert"	,ltnt_requestbuilder_insert	},
	{ "select"	,ltnt_requestbuilder_select	},
	{ "delete"	,ltnt_requestbuilder_delete	},
	{ "update"	,ltnt_requestbuilder_update	},
	{ "call"	,ltnt_requestbuilder_call	},
	{ "getvalue"	,ltnt_requestbuilder_getval	},
	{ "flush"	,ltnt_requestbuilder_flush	},
	{ "__gc"	,ltnt_requestbuilder_gc		},
	{ NULL		,NULL				}
};

/*
 * Register 'RequestBuilder' "class";
 */
int lrequestbuilder_open(lua_State *L) {
	luaL_newmetatable(L, "RequestBuilder");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lrequestbuilder_meta, 0);
	return 1;
}

/*
 * Register 'ResponseParser' "class";
 */
int lresponseparser_open(lua_State *L) {
	luaL_newmetatable(L, "ResponseParser");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	luaL_setfuncs(L, lresponseparser_meta, 0);
	return 1;
}

int luaopen_tnt(struct lua_State *L) {
	luaL_newlib(L, lrequestresponse);
	lrequestbuilder_open(L);
	lua_setfield(L, 3, "RequestBuilder");
	lresponseparser_open(L);
	lua_setfield(L, 3, "ResponseParser");
	lregister_enum(L, 3, ops, "ops");
	lregister_enum(L, 3, flags, "flags");
	/*
	 * Register other functions
	 */
	return 1;
}

