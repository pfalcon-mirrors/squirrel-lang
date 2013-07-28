#include <stdio.h>
#define SQSTD_MODULE_HELPERS
#include <sqstdmoduleapi.h>

_SQSTD_DECLARE_MODULE_API();

#ifdef _WIN32
#define SQ_TESTMODULE_API __declspec(dllexport)
#else
#define SQ_TESTMODULE_API extern
#endif

#ifdef _WIN32
#include <windows.h>
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	return TRUE;
}
#endif

int testmodule_printline(HSQUIRRELVM v)
{
	const SQChar *line;
	sq_getstring(v,2,&line);
	scprintf(_SC("%s\n"),line);
	return 0;
}

/*MODULE INTERFACE*/
SQ_TESTMODULE_API int sqstd_module_getapiversion()
{
	return SQSTD_MODULE_API_VERSION;
}

SQ_TESTMODULE_API int sqstd_module_charsize()
{
	return sizeof(SQChar);
}

SQ_TESTMODULE_API int sqstd_module_open(HSQUIRRELVM v,SQModuleAPI *sapi)
{
	_SQSTD_INIT_MODULE_API(sapi);
	sq_pushstring(v,_SC("printline"),-1);
	sq_newclosure(v,testmodule_printline,0);
	sq_setparamscheck(v,2,_SC(".s"));
	sq_rawset(v,-3);
	return SQ_OK;
}

SQ_TESTMODULE_API void sqstd_module_close()
{

}