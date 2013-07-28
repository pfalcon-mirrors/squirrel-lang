#include "sqstash.h"
#include "sqstashlib.h"
#ifdef _WIN32
#include <windows.h>
#define MODULE_HANDLE HMODULE
#define LOAD_MODULE(mod_name) (LoadLibrary(mod_name))
#define GET_PROC(hmod,procname) (GetProcAddress(hmod,procname))
#define FREE_MODULE(hmod) (FreeLibrary(hmod))
#endif
#ifdef _UNIX
#include <dlfcn.h>
#define MODULE_HANDLE void*
#define LOAD_MODULE(mod_name) (dlopen(mod_name, RTLD_LAZY))
#define GET_PROC(hmod,procname) (dlsym(hmod,procname))
#define FREE_MODULE(hmod) (dlclose(hmod))
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define STASH_GLOBAL "_STASHES_"



typedef int(*p_sq_stash_getapiversion)();
typedef int(*p_sq_stash_open)(HSQUIRRELVM,SQStashAPI *);
typedef int(*p_sq_stash_close)(HSQUIRRELVM); 

SQStashAPI g_StashAPI;
bool g_Initialized=false;


int sq_stashlib_close()
{
	return 0;
}

int sq_stash_loadstash(HSQUIRRELVM v,const SQChar *stash)
{
	p_sq_stash_getapiversion sq_stash_getapiversion=NULL;
	p_sq_stash_open sq_stash_open=NULL;
	p_sq_stash_close sq_stash_close=NULL;

	MODULE_HANDLE handle=NULL;
	bool create_stash_table=false;
	//sq_pushroottable(v);
	sq_pushstring(v,STASH_GLOBAL,strlen(STASH_GLOBAL));
	if(SQ_FAILED(sq_get(v,-2)))
	{
		sq_pop(v,1);
		return 0;
	}
	sq_pushstring(v,stash,strlen(stash));
	if(SQ_FAILED(sq_get(v,-2)))
	{
		handle=LOAD_MODULE(stash);
		if(!handle)
		{
			sq_pop(v,2);
			return 0;
		}
		sq_stash_getapiversion=(p_sq_stash_getapiversion)GET_PROC(handle,"sq_stash_getapiversion");
		sq_stash_open=(p_sq_stash_open)GET_PROC(handle,"sq_stash_open");
		sq_stash_close=(p_sq_stash_close)GET_PROC(handle,"sq_stash_close");
		

		if(!sq_stash_getapiversion
			|| !sq_stash_open
			|| !sq_stash_close)
		{
			FREE_MODULE(handle);
			sq_pop(v,2);
			return 0;
		}
		sq_pushstring(v,stash,strlen(stash));
		sq_newtable(v);
		sq_createslot(v,-3);
		sq_pushstring(v,stash,strlen(stash));
		sq_get(v,-2);
		sq_pushstring(v,"sq_stash_getapiversion",strlen("sq_stash_getapiversion"));
		sq_pushuserpointer(v,(SQUserPointer)sq_stash_getapiversion);
		sq_createslot(v,-3);
		sq_pushstring(v,"sq_stash_open",strlen("sq_stash_open"));
		sq_pushuserpointer(v,(SQUserPointer)sq_stash_open);
		sq_createslot(v,-3);
		sq_pushstring(v,"sq_stash_close",strlen("sq_stash_close"));
		sq_pushuserpointer(v,(SQUserPointer)sq_stash_close);
		sq_createslot(v,-3);
		sq_pushstring(v,"hmodule",strlen("hmodule"));
		sq_pushuserpointer(v,handle);
		sq_createslot(v,-3);
		

		sq_pop(v,3);

		sq_stash_open(v,&g_StashAPI); 
	}

	return 1;
}


int stashlib_loadstash(HSQUIRRELVM v)
{
	bool poptable=true;
	if(sq_gettop(v)<2)return sq_throwerror(v,"wrong number of params");
	const SQChar *stash;
	sq_getstring(v,2,&stash);
	if(sq_gettop(v)<3){
		sq_pushroottable(v);
		poptable=true;
	}
	
	sq_stash_loadstash(v,stash);
	if(poptable)sq_pop(v,1);
	return 1;
}

int sq_stashlib_register(HSQUIRRELVM v)
{
	if(g_Initialized)return 0;
	//sq_pushroottable(v);
	sq_pushstring(v,STASH_GLOBAL,strlen(STASH_GLOBAL));
	sq_newtable(v);
	sq_createslot(v,-3);
	
	sq_pushstring(v,"loadstash",strlen("loadstash"));
	sq_newclosure(v,stashlib_loadstash,0);
	sq_createslot(v,-3);
	//sq_pop(v,1);

	g_StashAPI._sq_compile=sq_compile;
	g_StashAPI._sq_push=sq_push;
	g_StashAPI._sq_pop=sq_pop;
	g_StashAPI._sq_remove=sq_remove;
	g_StashAPI._sq_gettop=sq_gettop;
	g_StashAPI._sq_cmp=sq_cmp;
	g_StashAPI._sq_newuserdata=sq_newuserdata;
	g_StashAPI._sq_newtable=sq_newtable;
	g_StashAPI._sq_newarray=sq_newarray;
	g_StashAPI._sq_newclosure=sq_newclosure;
	g_StashAPI._sq_pushstring=sq_pushstring;
	g_StashAPI._sq_pushfloat=sq_pushfloat;
	g_StashAPI._sq_pushinteger=sq_pushinteger;
	g_StashAPI._sq_pushuserpointer=sq_pushuserpointer;
	g_StashAPI._sq_pushnull=sq_pushnull;
	g_StashAPI._sq_gettype=sq_gettype;
	g_StashAPI._sq_getstring=sq_getstring;
	g_StashAPI._sq_getsize=sq_getsize;
	g_StashAPI._sq_getinteger=sq_getinteger;
	g_StashAPI._sq_getfloat=sq_getfloat;
	g_StashAPI._sq_getuserpointer=sq_getuserpointer;
	g_StashAPI._sq_getuserdata=sq_getuserdata;
	g_StashAPI._sq_setreleasehook=sq_setreleasehook;
	g_StashAPI._sq_getscratchpad=sq_getscratchpad;
	g_StashAPI._sq_pushroottable=sq_pushroottable;
	g_StashAPI._sq_set=sq_set;
	g_StashAPI._sq_get=sq_get;
	g_StashAPI._sq_rawset=sq_rawset;
	g_StashAPI._sq_rawget=sq_rawget;
	g_StashAPI._sq_createslot=sq_createslot;
	g_StashAPI._sq_arrayappend=sq_arrayappend;
	g_StashAPI._sq_setdelegate=sq_setdelegate;
	g_StashAPI._sq_getdelegate=sq_getdelegate;
	g_StashAPI._sq_setfreevariable=sq_setfreevariable;
	g_StashAPI._sq_call=sq_call;
	g_StashAPI._sq_getlocal=sq_getlocal;
	g_StashAPI._sq_getstackobj=sq_getstackobj;
	g_StashAPI._sq_pushobject=sq_pushobject;
	g_StashAPI._sq_addref=sq_addref;
	g_StashAPI._sq_release=sq_release;
	g_StashAPI._sq_resetobject=sq_resetobject;
	g_StashAPI._sq_throwerror=sq_throwerror;
	g_StashAPI._sq_writeclosure=sq_writeclosure;
	g_StashAPI._sq_readclosure=sq_readclosure;
	g_StashAPI._sq_malloc=sq_malloc;
	g_StashAPI._sq_realloc=sq_realloc;
	g_StashAPI._sq_free=sq_free;
	g_StashAPI._sq_blob_createdelegate=sq_blob_createdelegate;
	g_StashAPI._sq_blob_newblob=sq_blob_newblob;
	g_StashAPI._sq_blob_getblob=sq_blob_getblob;
	g_StashAPI._sq_blob_getblobsize=sq_blob_getblobsize;

	g_Initialized=true;

	return 1;
}

