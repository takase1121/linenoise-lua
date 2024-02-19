#===============================#
#      IMPORTANT VARIABLES      #
#===============================#
USE_SYSTEM_LUA ?= 0
STRIP ?= strip -x


SOSUFFIX := so
LIBS :=

ifeq ($(OS),Windows_NT)
	SOSUFFIX := dll
endif

ifeq ($(USE_SYSTEM_LUA),1)
	LIBS := -llua
endif

SONAME := linenoise_lua.$(SOSUFFIX)
OBJS := linenoise_lua.o linenoise/linenoise-amalgamation.o linenoise/linenoise-amalgamation.c
CFLAGS += -fPIC -Os

$(SONAME): linenoise_lua.o linenoise/linenoise-amalgamation.o
	$(CC) -shared $(LDFLAGS) -o $@ $^ $(LIBS)
	$(STRIP) $@

linenoise_lua.o: CFLAGS += -DUSE_SYSTEM_LUA=$(USE_SYSTEM_LUA) -Wall -Werror -std=c99
linenoise_lua.o: linenoise_lua.c linenoise/linenoise.h
linenoise/linenoise-amalgamation.o: linenoise/linenoise-amalgamation.c linenoise/linenoise.h
linenoise/linenoise-amalgamation.c: linenoise/utf8.h linenoise/utf8.c linenoise/stringbuf.h linenoise/stringbuf.c linenoise/linenoise.c
	@for i in $^; do echo "#line 1 \"$$i\""; cat $$i; done > $@

clean:
	$(RM) $(SONAME) $(OBJS)

.PHONY: clean