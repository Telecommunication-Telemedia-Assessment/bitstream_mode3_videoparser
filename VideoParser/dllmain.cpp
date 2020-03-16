// dllmain.cpp : Definiert den Einstiegspunkt für die DLL-Anwendung.
#include "stdafx.h"
#include "VideoParser.h"

BOOL APIENTRY DllMain( HMODULE hModule,  DWORD  ul_reason_for_call, LPVOID lpReserved )
  {
  time_t              rawtime ;
  struct              tm * timeinfo;

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
    time( &rawtime);
    timeinfo = localtime( &rawtime);

    printf ("\n\n********************* Starting VideoPars *#+********************************\n");
    printf ("********************* V.%3.3lf Build %05.2lf (%4.2lf.%4d) **********************\n\n", IAD_PROBE_VERSION, IAD_PROBE_BUILD, IAD_PROBE_DATE, IAD_PROBE_YEAR);
    printf ("Start Time: %s\n", asctime( timeinfo));
    break ;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

