#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lua.hpp"
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

const char*  __code =
"\
function __function(){ \
      __prototypes = {}; \
      setmetatable( __function, { \
                                    __index = function( k ) { \
                                        var t = __prototypes[this]; \
                                        if( not t ){ \
                                            t = {}; \
                                            __prototypes[this] = t; \
                                        } \
                                        return t; \
                                    } \
                                } \
                   ); \
  } \
__function(); \
__object_prototype.prototype = __object_prototype; \
\
\
\
_G['new'] = function(...){ \
  var obj = {}; \
  setmetatable( obj, {__index=this.prototype} ); \
  pcall3(obj,...); \
  return obj; \
} \
\
\
\
";

//\
//function new(f){ \
//  var obj = {}; \
//  pcall(f, obj); \
//  obj.prototype = f.prototype; \
//  return obj; \
//} \

static int run(lua_State* L, const char* buffer){
  unsigned long size = strlen(buffer);
  const char* cs = changeGrammar(buffer, &size);
  if( strlen(cs)!=size ) {
    printf("Error: strlen(cs)!=size: %ld!=%ld",strlen(cs), size);
  } else {
    printf("\n=========开始执行脚本=======\n");
  }
  int error = luaL_loadbufferx(L, cs, size, "Rokid", NULL);
  if (error) {
    const char* s = lua_tostring(L, -1);
    printf("[error] %s", s);
    return error;
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

int __new(lua_State* L){
  lua_checkstack(L, 32);
  int argN = lua_gettop(L)-1;
  if( lua_type(L, 1)==LUA_TFUNCTION ) {
    lua_remove(L, 2);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_insert(L, 2);
    lua_insert(L, 1);
    
    int errorCode = lua_pcall(L, argN, 0, 0);
    if ( errorCode != 0 ) {
      const char* s = lua_tostring(L, -1);
      printf("%s", s);
      return 0;
    }
  }
  return 1;
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
    
    {
        const char* tname = "__object_prototype";
        Table* t = (Table*)lua_createtable(L, 0, 2);  /* create metatable */
        lua_pushstring(L, tname);
        lua_setfield(L, -2, "__name");  /* metatable.__name = tname */
        lua_pushvalue(L, -1);
        lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
        L->obj_prototype = t;
    }
    
    lua_pushvalue(L, -1);
    lua_setglobal(L, "__object_prototype");
    
    lua_pushstring(L, "__index");//必须要的。
    lua_pushvalue(L, -2); /* pushes the metatable */
    lua_settable(L, -3); /* metatable.__index = metatable */
    

    
    lua_getglobal(L, "_G");
    lua_setglobal(L, "this");
    lua_getglobal(L, "print");
    lua_setglobal(L, "alert");
    
//    lua_pushcclosure(L, __new, 0);
//    lua_setglobal(L, "new");
    
    run(L, __code);
    return run(L, buffer);
  }
  
  return 0;
}

