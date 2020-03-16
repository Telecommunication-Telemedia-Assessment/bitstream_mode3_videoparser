// stdafx.h: Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#if defined WIN32
    #define WIN32_LEAN_AND_MEAN             // Selten verwendete Komponenten aus Windows-Headern ausschließen
    #include     "targetver.h"
    #include     <windows.h>
    #include     <tchar.h>
    #include     <direct.h>
    #include     <conio.h>
    #include     <stdint.h>

#endif

#include     <stdio.h>
#include     <stdio.h>
#include     <math.h>
#include     <string.h>
#include     <stdlib.h>

#if !defined BYTE 
  typedef unsigned char       BYTE;
#endif

#if !defined uint32_t
  typedef unsigned int       uint32_t ;
#endif 


