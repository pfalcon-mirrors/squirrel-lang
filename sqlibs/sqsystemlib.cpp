#include <squirrel.h>
#include <time.h>
#include <stdlib.h>
#include "sqsystemlib.h"

#ifdef SQUNICODE
#define scgetenv _wgetenv
#define scsystem _wsystem
#define scasctime _wasctime
#else
#define scgetenv getenv
#define scsystem system
#define scasctime asctime
#endif
int system_getenv(HSQUIRRELVM v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
        sq_pushstring(v,scgetenv(s),-1);
		return 1;
	}
	return 0;
}

int system_system(HSQUIRRELVM v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		sq_pushinteger(v,scsystem(s));
		return 1;
	}
	return sq_throwerror(v,_SC("wrong param"));
}


int system_clock(HSQUIRRELVM v)
{
	sq_pushfloat(v,((SQFloat)clock())/(SQFloat)CLOCKS_PER_SEC);
	return 1;
}

int system_gmtime(HSQUIRRELVM v)
{
	time_t t;
	tm *newtime;
	time(&t);
	newtime=gmtime(&t);
	sq_pushstring(v,scasctime(newtime),-1);
	return 1;
}

int system_localtime(HSQUIRRELVM v)
{
	time_t t;
	tm *newtime;
	time(&t);
	newtime=localtime(&t);
	sq_pushstring(v,scasctime(newtime),-1);
	return 1;
}



#define _DECL_FUNC(name,nparams) {_SC(#name),system_##name,nparams}
static SQRegFunction systemlib_funcs[]={
	_DECL_FUNC(getenv,2),
	_DECL_FUNC(system,2),
	_DECL_FUNC(clock,1),
	_DECL_FUNC(gmtime,1),
	_DECL_FUNC(localtime,1),
	{0,0}
};


int sq_systemlib_register(HSQUIRRELVM v)
{
	int i=0;
	while(systemlib_funcs[i].name!=0)
	{
		sq_pushstring(v,systemlib_funcs[i].name,-1);
		sq_newclosure(v,systemlib_funcs[i].f,systemlib_funcs[i].nparamscheck,0);
		sq_createslot(v,-3);
		i++;
	}
	return 1;
}
