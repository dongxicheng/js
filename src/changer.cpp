
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


#ifndef YES
#define YES 1
#endif

#ifndef NO
#define NO 0
#endif

#ifndef BOOL
#define BOOL int
#endif


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

inline static BOOL currentChar(const unsigned char* cs, long i, long length, char c){
    if( i<length && cs[i]==c ) {
        return YES;
    }
    return NO;
}

static std::map<std::string,bool> map;
static bool mapInited = false;

static void mapInit(){
    if( mapInited ) {
        return ;
    }
    mapInited = true;
    map["return"] = true;
    map["for"] = true;
    map["if"] = true;
    map["elseif"] = true;
    map["while"] = true;
    map["until"] = true;
    map["in"] = true;
    map["and"] = true;
    map["break"] = true;
    map["goto"] = true;
    map["not"] = true;
    map["or"] = true;
    map["repeat"] = true;
}

/*
 * 转换成标准lua语法
 */
const char* changeGrammar(const char* data0, unsigned long* pLength){
    const unsigned char* data = (const unsigned char*)data0;
    unsigned long length = *pLength;
    {
        static BOOL inited = NO;
        if( !inited ) {
            inited = YES;
            charTypesInited();
            mapInit();
        }
    }
    if( data && length>0 ) {
        unsigned long csLen = length;
        unsigned char* cs = (unsigned char*)malloc(csLen+16);
        memcpy(cs, data, csLen);
        cs[csLen] = 0;
        
        char* thisArr = (char*) malloc(csLen+16);
        memset(thisArr, 0, csLen);
        long thisNum = 0;
        
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
                    if( map[name] ) {
                        // 非函数
                        break;
                    }
                    i = skipSpace(cs, i, length);
                    if( currentChar(cs, i, length, '(') ) {
                        if( currentChar(cs, i+1, length, ')') ) {
                            thisArr[i++] = 1; thisNum++;
                        } else {
                            thisArr[i++] = 2; thisNum++;
                        }
                    }
                    break;
                }
                case LV_TYPE_CALL_POINT:{
                    i++;
                    if( currentChar(cs, i, length, '(') ) {
                        if( currentChar(cs, i+1, length, ')') ) {
                            thisArr[i++] = 1; thisNum++;
                        } else {
                            thisArr[i++] = 2; thisNum++;
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
                                    cs[i0] = ':';
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
        long cs2Len = csLen + thisNum*THIS_LEN;
        unsigned char* cs2 = (unsigned char*) malloc(cs2Len+16);
        memset(cs2, 0, cs2Len+16);
        long i=0;
        long j=0;
        for(;i<length;) {
            cs2[j++] = cs[i++];
            if( thisArr[i-1]==1 ) {
                memcpy(cs2+j, THIS, THIS_LEN-1);
                j += THIS_LEN-1;
            } else if( thisArr[i-1]==2 ) {
                memcpy(cs2+j, THIS, THIS_LEN);
                j += THIS_LEN;
            }
        }
        free(cs);
        *pLength = j;
        printf("changer: %s\n", cs2);
        return (const char*)cs2;
    }
    return 0;
}
