USE_SYSTEM_LUA := 0

SOSUFFIX := so
WIN32_SUPPORT :=

ifeq ($(OS),Windows_NT)
	SOSUFFIX := dll
	WIN32_SUPPORT := linenoise/linenoise-win32.c
endif

LUA_LIBS_0 :=
LUA_LIBS_1 := -llua
LIBS := $(LUA_LIBS_$(USE_SYSTEM_LUA))
CFLAGS += -fPIC -O2

SONAME := linenoise_lua.$(SOSUFFIX)
OBJS := linenoise_lua.o linenoise/linenoise-amalgamation.o linenoise/linenoise-amalgamation.c

$(SONAME): linenoise_lua.o linenoise/linenoise-amalgamation.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)

linenoise_lua.o: CFLAGS += -DUSE_SYSTEM_LUA=$(USE_SYSTEM_LUA) -Wall -Werror -std=c99
linenoise_lua.o: linenoise_lua.c linenoise/linenoise.h
linenoise/linenoise-amalgamation.o: linenoise/linenoise-amalgamation.c linenoise/linenoise.h
linenoise/linenoise-amalgamation.c: linenoise/utf8.h linenoise/utf8.c linenoise/stringbuf.h linenoise/stringbuf.c linenoise/linenoise.c $(WIN32_SUPPORT)
	@for i in $^; do echo "#line 1 \"$$i\""; cat $$i; done > $@

clean:
	$(RM) $(SONAME) $(OBJS)

.PHONY: clean