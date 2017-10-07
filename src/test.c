#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "changer.h"

static void * my_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
    (void)ud;
    (void)osize;
    if (nsize == 0) {
        free(ptr);
        return NULL;
    } else {
        return realloc(ptr, nsize);
    }
}

int main(int argc, const char** args) {
    if( argc>=2 ) {
        static char buffer[1024*100] = {0};
        const char* fileName = args[1];
        FILE* f = fopen(fileName, "r");
        fread(buffer, sizeof(buffer), 1, f);
        fclose(f);
        
        lua_State* L = lua_newstate(my_alloc, NULL);
        luaL_openlibs(L);
        lua_getglobal(L, "_G");
        lua_setglobal(L, "this");
        
        unsigned long size = strlen(buffer);
        const char* cs = changeGrammar(buffer, &size);
        
        int error = luaL_loadbufferx(L, cs, size-1, "Rokid", NULL);
        if (error) {
            const char* s = lua_tostring(L, -1);
            printf("[error] %s", s);
        } else {
            if( L && lua_type(L, -1) == LUA_TFUNCTION ) {
                
                int errorCode = lua_pcall( L, 0, 0, 0);
                if ( errorCode != 0 ) {
                    const char* s = lua_tostring(L, -1);
                    printf("%s", s);
                    return 0;
                }
                return 0;
            }
            return 1;
        }
    }
    
    return 0;
}
