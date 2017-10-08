
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <memory>
#include <mutex>
#include <thread>
#include <stdlib.h>
#include <string>
#include <vector>
#include <map>

#import "changer.h"

// ‘.’ 和‘:’ 相互转化代码
#define LV_TYPE_WORD_FIRST    (1)
#define LV_TYPE_WORD_SECOND   (2)
#define LV_TYPE_NUMBER        (4)
#define LV_TYPE_CHAR_SPACE    (8)
#define LV_TYPE_CHAR_NOTES    (16)
#define LV_TYPE_CHAR_POINT    (32)
#define LV_TYPE_CALL_POINT    (64)

static int g_charTypes[256] = {0};
static void charTypesInited(){
  g_charTypes['_'] = LV_TYPE_WORD_FIRST|LV_TYPE_WORD_SECOND;
  for( char c = 'a'; c<='z'; c++ ) {
    g_charTypes[c] = LV_TYPE_WORD_FIRST|LV_TYPE_WORD_SECOND;
  }
  for( char c = 'A'; c<='Z'; c++ ) {
    g_charTypes[c] = LV_TYPE_WORD_FIRST|LV_TYPE_WORD_SECOND;
  }
  for( char c = '0'; c<='9'; c++ ) {
    g_charTypes[c] = LV_TYPE_NUMBER | LV_TYPE_WORD_SECOND;
  }
  g_charTypes[' '] = LV_TYPE_CHAR_SPACE;
  g_charTypes['\n'] = LV_TYPE_CHAR_SPACE;
  g_charTypes['.'] = LV_TYPE_CHAR_POINT;
  g_charTypes[':'] = LV_TYPE_CHAR_POINT;
  g_charTypes['-'] = LV_TYPE_CHAR_NOTES;
  g_charTypes[')'] = LV_TYPE_CALL_POINT;
  g_charTypes[']'] = LV_TYPE_CALL_POINT;
}

inline static long skipNotes(const unsigned char* cs, long i, long length){
  for( int m=0 ; m<2 && i<length; m++ ) {
    char c = cs[i];
    if( c=='-' ) {
      i++;
    } else {
      return i;
    }
  }
  for( ;i<length;) {
    char c = cs[i];
    if( c=='\n' ) {
      i++;
      return i;
    } else {
      return i;
    }
  }
  return i;
}

inline static long skipName(const unsigned char* cs, long i, long length){
  int* types = g_charTypes;
  if( i<length ) {
    char c = cs[i];
    int type = types[c];
    if( type&LV_TYPE_WORD_FIRST ) {
      i++;
    } else {
      return i;
    }
  }
  for( ;i<length;) {
    char c = cs[i];
    int type = types[c];
    if( type&LV_TYPE_WORD_SECOND ) {
      i++;
    } else {
      return i;
    }
  }
  return i;
}

inline static long skipNumber(const unsigned char* cs, long i, long length){
  int* types = g_charTypes;
  for( ;i<length;) {
    char c = cs[i];
    int type = types[c];
    if( type&LV_TYPE_WORD_SECOND ) {
      i++;
    } else {
      return i;
    }
  }
  return i;
}

inline static long skipSpace(const unsigned char* cs, long i, long length){
  int* types = g_charTypes;
  for( ;i<length;) {
    unsigned char c = cs[i];
    int type = types[c];
    if( type&LV_TYPE_CHAR_SPACE) {
      i++;
    } else {
      return i;
    }
  }
  return i;
}

inline static long skipOther(const unsigned char* cs, long i, long length){
  int* types = g_charTypes;
  for( ;i<length;) {
    unsigned char c = cs[i];
    int type = types[c];
    if( type==0) {
      i++;
    } else {
      return i;
    }
  }
  return i;
}

inline static bool currentChar(const unsigned char* cs, long i, long length, char c){
  if( i<length && cs[i]==c ) {
    return true;
  }
  return false;
}

static std::map<std::string,bool> keyWordMap;
static bool mapInited = false;

static void mapInit(){
  if( mapInited ) {
    return ;
  }
  mapInited = true;
  keyWordMap["return"] = true;
  keyWordMap["for"] = true;
  keyWordMap["if"] = true;
  keyWordMap["elseif"] = true;
  keyWordMap["while"] = true;
  keyWordMap["until"] = true;
  keyWordMap["in"] = true;
  keyWordMap["and"] = true;
  keyWordMap["break"] = true;
  keyWordMap["goto"] = true;
  keyWordMap["not"] = true;
  keyWordMap["or"] = true;
  keyWordMap["repeat"] = true;
}

typedef enum {
  CHANGE_TYPE_NONE = 0,
  CHANGE_TYPE_INSERT1 = 1,
  CHANGE_TYPE_INSERT2 = 2,
  CHANGE_TYPE_REPLACE = 3,
} EnumChangeType;

const char* changeGrammar(const char* data, unsigned long* pLength){
  const unsigned long length = *pLength;
  {
    static bool inited = false;
    if( !inited ) {
      inited = true;
      charTypesInited();
      mapInit();
    }
  }
  if( data && length>0 ) {
    const unsigned char* cs = (const unsigned char*)data;
    long size = (length+16)*sizeof(int);
    int* changeTypes = (int*) malloc( size );
    memset(changeTypes, 0, size );
    long changeNum = 0;
    
    for ( long i=0; i<length;) {
      unsigned char c = cs[i];
      int type = g_charTypes[c];
      switch (type) {
        case LV_TYPE_CHAR_NOTES:
          i = skipNotes(cs, i, length);
          break;
        case LV_TYPE_CHAR_SPACE:
          i = skipSpace(cs, i, length);
          break;
        case LV_TYPE_NUMBER:
        case LV_TYPE_NUMBER|LV_TYPE_WORD_SECOND: {
          i = skipNumber(cs, i, length);
          break;
        }
        case LV_TYPE_WORD_FIRST:
        case LV_TYPE_WORD_SECOND:
        case LV_TYPE_WORD_FIRST|LV_TYPE_WORD_SECOND: {
          unsigned long j = i;
          i = skipName(cs, i, length);
          unsigned long k = i;
          std::string name( (const char*)cs+j, k-j);
          if( keyWordMap[name] ) {
            // 非函数
            break;
          }
          i = skipSpace(cs, i, length);
          if( currentChar(cs, i, length, '(') ) {
            if( currentChar(cs, i+1, length, ')') ) {
              changeTypes[i++] = CHANGE_TYPE_INSERT1;
              changeNum++;
            } else {
              changeTypes[i++] = CHANGE_TYPE_INSERT2;
              changeNum++;
            }
          }
          break;
        }
        case LV_TYPE_CALL_POINT:{
          i++;
          if( currentChar(cs, i, length, '(') ) {
            if( currentChar(cs, i+1, length, ')') ) {
              changeTypes[i++] = CHANGE_TYPE_INSERT1;
              changeNum++;
            } else {
              changeTypes[i++] = CHANGE_TYPE_INSERT2;
              changeNum++;
            }
          }
          break;
        }
        case LV_TYPE_CHAR_POINT:{
          if ( currentChar(cs, i, length, '.') && currentChar(cs, i+1, length, '.') ) {
            i += 2;
          } else if ( currentChar(cs, i, length, '.') || currentChar(cs, i, length, ':') ) {
            long i0 = i;
            i++;
            i = skipSpace(cs, i, length);
            long i2 = skipName(cs, i, length);
            if( i2>i ) {
              std::string name( (const char*)cs+i, i2-i);
              if( name=="function" ) {
                // 匿名函数
                break;
              }
              i = i2;
              i = skipSpace(cs, i, length);
              if( currentChar(cs, i, length, '(') ) {
                unsigned char tempChar = cs[i0];
                if( (char)tempChar=='.' ) {
                  changeTypes[i0] = CHANGE_TYPE_REPLACE;
                }
                break;
              }
            }
          }
          break;
        }
          
        default:
          i = skipOther(cs, i, length);
          break;
      }
      
    }
    const char* THIS = "this,";
    const unsigned long THIS_LEN = strlen(THIS);
    long newLength = length + changeNum*THIS_LEN;
    unsigned char* newChars = (unsigned char*) malloc(newLength+16);
    memset(newChars, 0, newLength+16);
    long i=0;
    long j=0;
    for(;i<length;) {
      long i0 = i;
      newChars[j++] = cs[i++];
      switch (changeTypes[i0]) {
        case CHANGE_TYPE_NONE:
          break;
        case CHANGE_TYPE_REPLACE:
          newChars[j-1] = ':';
          break;
        case CHANGE_TYPE_INSERT1:
          memcpy(newChars+j, THIS, THIS_LEN-1);
          j += THIS_LEN-1;
          break;
        case CHANGE_TYPE_INSERT2:
          memcpy(newChars+j, THIS, THIS_LEN);
          j += THIS_LEN;
          break;
        default:
          printf("[Error] changeTypes[i0] \n");
          break;
      }
    }
    
    *pLength = j;
    printf("JavaScript: %s\n", newChars);
    free(changeTypes);
    return (const char*)newChars;
  }
  return 0;
}

