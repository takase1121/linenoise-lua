#define LITE_XL_PLUGIN_ENTRYPOINT

#include <string.h>
#include <stdlib.h>

#include "linenoise/linenoise.h"

#if USE_SYSTEM_LUA == 1
#include <lua.h>
#include <lauxlib.h>
#else
#include "lite_xl_plugin_api.h"
#endif

#define LINENOISE_COMPLETION_CALLBACK_NAME "__LINENOISE_COMPLETION_FALLBACK__"
#define LINENOISE_HINT_CALLBACK_NAME "__LINENOISE_HINT_FALLBACK__"

#if LUA_VERSION_NUM == 501
#define lua_table_getn lua_objlen
#else
#define lua_table_getn luaL_len
#endif

static void completion_callback(const char *prefix, linenoiseCompletions *comp, void *userdata) {
    lua_State *L = (lua_State *) userdata;
    if (lua_getfield(L, LUA_REGISTRYINDEX, LINENOISE_COMPLETION_CALLBACK_NAME) == LUA_TFUNCTION) {
        lua_pushstring(L, prefix);
        int result = lua_pcall(L, 2, 1, 0);
        if (result == LUA_OK) {
            if (lua_type(L, -1) == LUA_TTABLE) {
                size_t len = lua_table_getn(L, -1);
                for (size_t i = 1; i <= len; i++) {
                    if (lua_rawgeti(L, -1, i) == LUA_TSTRING)
                        linenoiseAddCompletion(comp, lua_tostring(L, -1));
                    lua_pop(L, 1);
                }
            }
        }
        if (result == LUA_OK || result == LUA_ERRRUN) lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

static int f_set_completion_callback(lua_State *L) {
    lua_settop(L, 1);
    if (lua_isnoneornil(L, 1)) {
        linenoiseSetCompletionCallback(NULL, NULL);
    } else {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_setfield(L, LUA_REGISTRYINDEX, LINENOISE_COMPLETION_CALLBACK_NAME);
        linenoiseSetCompletionCallback(&completion_callback, (void *) L);
    }
    return 0;
}

static char *hints_callback(const char *buf, int *color, int *bold, void *userdata) {
    char *result_str = NULL;
    lua_State *L = (lua_State *) userdata;
    if (lua_getfield(L, LUA_REGISTRYINDEX, LINENOISE_HINT_CALLBACK_NAME) == LUA_TFUNCTION) {
        lua_pushstring(L, buf);
        lua_pushinteger(L, *color);
        lua_pushinteger(L, *bold);
        int result = lua_pcall(L, 2, 3, 0);
        if (result == LUA_OK) {
            if (lua_isinteger(L, -1))
                *bold = lua_tointeger(L, -1);
            if (lua_isinteger(L, -2))
                *color = lua_tointeger(L, -2);
            if (lua_isstring(L, -3)) {
                size_t len = 0;
                const char *r = lua_tolstring(L, -3, &len);
                result_str = calloc(sizeof(char), len);
                if (result_str)
                    memcpy(result_str, r, len);
            }
            lua_pop(L, 3);
        } else if (result == LUA_ERRRUN) {
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return result_str;
}

static void hints_free_callback(void *data, void *userdata) {
    if (data) free(data);
}

static int f_set_hints_callback(lua_State *L) {
    lua_settop(L, 1);
    if (lua_isnoneornil(L, 1)) {
        linenoiseSetHintsCallback(NULL, NULL);
        linenoiseSetFreeHintsCallback(NULL);
    } else {
        luaL_checktype(L, 1, LUA_TFUNCTION);
        lua_setfield(L, LUA_REGISTRYINDEX, LINENOISE_HINT_CALLBACK_NAME);
        linenoiseSetHintsCallback(&hints_callback, (void *) L);
        linenoiseSetFreeHintsCallback(&hints_free_callback);
    }
    return 0;
}

static int f_linenoise(lua_State *L) {
    const char *prompt = luaL_checkstring(L, 1);
    const char *initial = luaL_optstring(L, 1, "");
    char *result = linenoiseWithInitial(prompt, initial);
    if (result)
        lua_pushstring(L, result);
    else
        lua_pushnil(L);
    return 1;
}

static int f_clear_screen(lua_State *L) {
    linenoiseClearScreen();
    return 0;
}

static int f_history_add(lua_State *L) {
    lua_pushboolean(L, linenoiseHistoryAdd(luaL_checkstring(L, 1)));
    return 1;
}

static int f_history_set_max_len(lua_State *L) {
    lua_pushboolean(L, linenoiseHistorySetMaxLen(luaL_checkinteger(L, 1)));
    return 1;
}

static int f_history_get_max_len(lua_State *L) {
    lua_pushinteger(L, linenoiseHistoryGetMaxLen());
    return 1;
}

static int f_history_save(lua_State *L) {
    lua_pushboolean(L, linenoiseHistorySave(luaL_checkstring(L, 1)) == -1 ? 0 : 1);
    return 1;
}

static int f_history_load(lua_State *L) {
    lua_pushboolean(L, linenoiseHistoryLoad(luaL_checkstring(L, 1)) == -1 ? 0 : 1);
    return 1;
}

static int f_history_free(lua_State *L) {
    linenoiseHistoryFree();
    return 0;
}

static int f_history(lua_State *L) {
    int len = 0;
    char **history = linenoiseHistory(&len);
    lua_createtable(L, len, 0);
    for (int i = 0; i < len; i++) {
        lua_pushstring(L, history[i]);
        lua_rawseti(L, -2, i+1);
    }
    return 1;
}

static int f_columns(lua_State *L) {
    lua_pushinteger(L, linenoiseColumns());
    return 1;
}

static int f_set_multiline(lua_State *L) {
    linenoiseSetMultiLine(lua_toboolean(L, 1));
    return 0;
}

int luaopen_linenoise_lua(lua_State *L) {
    lua_newtable(L);
    lua_pushcfunction(L, f_set_completion_callback); lua_setfield(L, -2, "set_completion_callback");
    lua_pushcfunction(L, f_set_hints_callback);      lua_setfield(L, -2, "set_hints_callback");
    lua_pushcfunction(L, f_linenoise);               lua_setfield(L, -2, "linenoise");
    lua_pushcfunction(L, f_clear_screen);            lua_setfield(L, -2, "clear_screen");
    lua_pushcfunction(L, f_history_add);             lua_setfield(L, -2, "history_add");
    lua_pushcfunction(L, f_history_set_max_len);     lua_setfield(L, -2, "history_set_max_len");
    lua_pushcfunction(L, f_history_get_max_len);     lua_setfield(L, -2, "history_get_max_len");
    lua_pushcfunction(L, f_history_save);            lua_setfield(L, -2, "history_save");
    lua_pushcfunction(L, f_history_load);            lua_setfield(L, -2, "history_load");
    lua_pushcfunction(L, f_history_free);            lua_setfield(L, -2, "history_free");
    lua_pushcfunction(L, f_history);                 lua_setfield(L, -2, "history");
    lua_pushcfunction(L, f_columns);                 lua_setfield(L, -2, "columns");
    lua_pushcfunction(L, f_set_multiline);           lua_setfield(L, -2, "set_multiline");
    return 1;
}