
#pragma once

#include     <math.h>

#if defined WIN32
  #define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschlieﬂen
  #include     <windows.h>
  #include     "targetver.h"
  #include     <inttypes.h>
  #include     <direct.h>
  #include     <conio.h>
  #include     <wtypes.h>
  #include     <assert.h>


  #ifdef VIDEOPARSER_EXPORTS
    #define VIDEOPARSER_API __declspec(dllexport)
  #else
    #define VIDEOPARSER_API __declspec(dllimport)
  #endif
  #define ErrorReturn( _txt1_, _txt2_, _x_){ if( _txt2_ != NULL) fprintf( stderr, _txt1_, _txt2_) ;else fprintf( stderr, _txt1_) ; return( _x_) ; }
  #define EMMS()  __asm{ emms }

#else

  #define VIDEOPARSER_API
  #define TRUE    1
  #define FALSE   0
  #define BOOL    bool
  typedef   unsigned char  BYTE ;

  #define ErrorReturn( _txt1_, _txt2_, _x_){return( _x_);}
  #define EMMS() asm ("emms")
  #define min(x, y) (x < y) ? x : y
  #define max(x, y) (x > y) ? x : y
#endif

#define ICLIP( _max, _min, _val ) ( min( max((_min),(_val)) ,(_max)))

#include     <stdint.h>
#include     <assert.h>
#include     <time.h>
#include     <stdio.h>
#include     <string.h>
#include     <stdlib.h>
