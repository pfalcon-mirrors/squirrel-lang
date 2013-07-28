#include <squirrel.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqstdmodule.h>
#include <sqstdmoduleapi.h>

#ifdef _WIN32
#define REL_NUT_FMT _SC("%s%s.nut")
#define ABS_NUT_FMT _SC("%s\\%s%s.nut")
#define REL_MODULE_FMT _SC("%ssq_%s.dll")
#define ABS_MODULE_FMT _SC("%s\\%ssq_%s.dll")
#else
#define REL_NUT_FMT _SC("%s%s.nut")
#define ABS_NUT_FMT _SC("%s/%s%s.nut")
#define REL_MODULE_FMT _SC("%slibsq_%s.so")
#define ABS_MODULE_FMT _SC("%s/%slibsq_%s.so")
#endif

#ifdef _UNICODE
#define scgetenv _wgetenv
#else
#define scgetenv getenv
#endif

#define SQSTD_MODULE_REGISTRY _SC("std_module_registry")
#define SQSTD_MODULE_PATHVAR _SC("SQSTDMODULEPATH")
#define SQSTD_MODULE_PATHSLOT _SC("std_module_path")
#define SQSTD_MODULE_USERAPISLOT _SC("std_module_userapi")

typedef int(*p_sqstd_module_getapiversion)();
typedef int(*p_sqstd_module_charsize)(int charsize);
typedef int(*p_sqstd_module_open)(HSQUIRRELVM,SQModuleAPI *);
typedef void(*p_sqstd_module_close)(); 


typedef struct tagSQBinaryModuleInterface{
	p_sqstd_module_getapiversion _sqstd_module_getapiversion;
	p_sqstd_module_charsize _sqstd_module_charsize;
	p_sqstd_module_open _sqstd_module_open;
	p_sqstd_module_close _sqstd_module_close;
	SQSTDHMODULE _module;
	SQSTDUserModuleAPI _api;
} SQBinaryModuleInterface;


static SQModuleAPI _module_api = {
	sq_open,
	sq_newthread,
	sq_seterrorhandler,
	sq_close,
	sq_setforeignptr,
	sq_getforeignptr,
	sq_setprintfunc,
	sq_suspendvm,
	sq_wakeupvm,
	sq_getvmstate,

/*compiler*/
	sq_compile,
	sq_compilebuffer,
	sq_enabledebuginfo,
	sq_setcompilererrorhandler,

/*stack operations*/
	sq_push,
	sq_pop,
	sq_remove,
	sq_gettop,
	sq_settop,
	sq_reservestack,
	sq_cmp,
	sq_move,

/*object creation handling*/
	sq_newuserdata,
	sq_newtable,
	sq_newarray,
	sq_newclosure,
	sq_setparamscheck,
	sq_pushstring,
	sq_pushfloat,
	sq_pushinteger,
	sq_pushuserpointer,
	sq_pushnull,
	sq_gettype,
	sq_getsize,
	sq_getstring,
	sq_getinteger,
	sq_getfloat,
	sq_getuserpointer,
	sq_getuserdata,
	sq_settypetag,
	sq_setreleasehook,
	sq_getscratchpad,

/*object manipulation*/
	sq_pushroottable,
	sq_pushregistrytable,
	sq_setroottable,
	sq_createslot,
	sq_deleteslot,
	sq_set,
	sq_get,
	sq_rawget,
	sq_rawset,
	sq_arrayappend,
	sq_arraypop,
	sq_arrayresize,
	sq_arrayreverse,
	sq_setdelegate,
	sq_getdelegate,
	sq_clone,
	sq_setfreevariable,
	sq_next,

/*calls*/
	sq_call,
	sq_resume,
	sq_getlocal,
	sq_throwerror,
	sq_getlasterror,

/*raw object handling*/
	sq_getstackobj,
	sq_pushobject,
	sq_addref,
	sq_release,
	sq_resetobject,

/*GC*/
	sq_collectgarbage,

/*serialization*/
	sq_writeclosure,
	sq_readclosure,

	sq_malloc,
	sq_realloc,
	sq_free,

/*debug*/
	sq_stackinfos,
	sq_setdebughook,

/*sqstd io*/
	sqstd_fopen,
	sqstd_fread,
	sqstd_fwrite,
	sqstd_fseek,
	sqstd_ftell,
	sqstd_fflush,
	sqstd_fclose,
	sqstd_feof,

	sqstd_createfile,
	sqstd_getfile,

/*sqstd blob*/
	sqstd_createblob,
	sqstd_getblob,
	sqstd_getblobsize
};

static SQInteger _sqstd_module_lexfeedASCII(SQUserPointer file)
{
	int ret;
	char c;
	if( ( ret=sqstd_fread(&c,sizeof(c),1,(SQFILE)file )>0) )
		return c;
	return 0;
}

static SQInteger _sqstd_module_lexfeedWCHAR(SQUserPointer file)
{
	int ret;
	unsigned short c;
	if( ( ret=sqstd_fread(&c,sizeof(c),1,(SQFILE)file )>0) )
		return (SQChar)c;
	return 0;
}

static int _sqstd_module_release_hook(SQUserPointer p,int size)
{
	SQBinaryModuleInterface *imod = (SQBinaryModuleInterface *)p;
	imod->_sqstd_module_close();
	imod->_api._sqstd_moduleapi_closemodule(imod->_module);
	return 1;
}

static int _sqstd_module_init(HSQUIRRELVM v)
{
	SQBinaryModuleInterface *imod;
	if(SQ_FAILED(sq_getuserdata(v,-1,(SQUserPointer*)&imod,NULL)))
		return sq_throwerror(v,_SC("internal module error"));
	sq_push(v,1); //repush the this
	if(SQ_FAILED(imod->_sqstd_module_open(v,&_module_api)))
		return SQ_ERROR;
	return 0;
}

static SQRESULT process_modulename(HSQUIRRELVM v,const SQChar *module)
{
	int n = 0;
	int namestart = 0;
	SQChar *t = sq_getscratchpad(v,((SQInteger)scstrlen(module)+1)*sizeof(SQChar));
	if(module[0] == '.')
		return sq_throwerror(v,_SC("module names cannot start with '.'"));
	while(module[n]!='\0') {
		if(module[n] == '/' ||
			module[n] == '\\') {
			return sq_throwerror(v,_SC("module name cannot contain('\' or '//')"));
		}
		if(module[n] != '.') {
			t[n] = module[n];
		}
		else {
			if(module[n+1]=='.' || module[n+1]=='\0')
				return sq_throwerror(v,_SC("invalid module name format"));
			namestart = n+1;
			t[n] = '/';
		}
		n++;
	}
	if(n == 0) return sq_throwerror(v,_SC("empty module name"));
	if(namestart == 0) {
		sq_pushstring(v,_SC(""),-1); //no realtive path
		sq_pushstring(v,t,n); //module name
	}
	else {
		sq_pushstring(v,t,namestart); //no realtive path
		sq_pushstring(v,&t[namestart],n-(namestart)); //module name
	}
	return SQ_OK;
}

#include <assert.h>
SQRESULT sqstd_importmodule(HSQUIRRELVM v, const SQChar* module)
{
	int top = sq_gettop(v);
	SQSTDUserModuleAPI *temp,api;
	const SQChar *basepath;
	sq_pushregistrytable(v);
	sq_pushstring(v,SQSTD_MODULE_REGISTRY,-1);
	if(SQ_FAILED(sq_rawget(v,-2))) {
		sq_settop(v,top);
		return sq_throwerror(v,_SC("the module lib not initialized"));
	}
	sq_pushstring(v,module,-1);
	if(SQ_SUCCEEDED(sq_rawget(v,-2))) {
		if(sq_gettype(v,-1) != OT_NULL) {
			//the module is already registered just return the init() again
			return SQ_OK;
		}
		else {
			//the entry is null so has to be reloaded
			sq_pop(v,1);
		}
	}
	sq_pop(v,1); //pops the module registry
	sq_pushstring(v,SQSTD_MODULE_USERAPISLOT,-1);
	if(SQ_FAILED(sq_rawget(v,-2))
	|| SQ_FAILED(sq_getuserdata(v,-1,(SQUserPointer *)&temp,NULL))) {
		sq_settop(v,top);
		return sq_throwerror(v,_SC("the module lib not initialized"));
	}
	api = *temp;
	sq_pop(v,1); //pops the api struct
	sq_pushstring(v,SQSTD_MODULE_PATHSLOT,-1);
	if(SQ_FAILED(sq_rawget(v,-2))
	|| SQ_FAILED(sq_getstring(v,-1,&basepath))) {
		sq_settop(v,top);
		return sq_throwerror(v,_SC("the module lib not initialized properly"));
	}
	sq_pop(v,1); // pops the path
	if(SQ_FAILED(process_modulename(v,module))) {
		sq_settop(v,top);
		return SQ_ERROR;
	}
	const SQChar *modulepath;
	const SQChar *modulename;
	sq_getstring(v,-2,&modulepath);
	sq_getstring(v,-1,&modulename);
	SQChar *path = sq_getscratchpad(v,(SQInteger)(scstrlen(basepath)+6+scstrlen(module))*sizeof(SQChar));
	SQFILE puremodule = NULL;
	scsprintf(path,REL_NUT_FMT,modulepath,modulename); //tries the local dir
	puremodule = sqstd_fopen(path,_SC("rb"));
	if(!puremodule) {
		scsprintf(path,ABS_NUT_FMT,basepath,modulepath,modulename); //tries the module dir
		puremodule = sqstd_fopen(path,_SC("rb"));
	}
	if(puremodule) { //LOAD PURE NUT
		sq_pop(v,2); //pops the strings of the path
		sq_pop(v,1); //pops the registry

		int ret;
		unsigned short uc;
		SQLEXREADFUNC func = _sqstd_module_lexfeedASCII;
		if((ret = sqstd_fread(&uc, 1, 2, puremodule))) {
			if(ret == 2 && uc == 0xFEFF)
				func = _sqstd_module_lexfeedWCHAR;
			else
				sqstd_fseek(puremodule, 0, SQ_SEEK_SET);
		}
		if(SQ_FAILED(sq_compile(v, func, puremodule, module, 0))) {
			sq_settop(v,top);
			sqstd_fclose(puremodule);
			return sq_throwerror(v,_SC("cannot compile stub file"));
		}
	//	sq_push(v,-2); //trg
	//	if(SQ_FAILED(sq_call(v,1,0))) {
	//		sq_settop(v,top);
	//		sqstd_fclose(puremodule);
	//		return SQ_ERROR;
	//	}
		//closure -2
		//reg at -1
		sq_pushregistrytable(v);
		sq_pushstring(v,SQSTD_MODULE_REGISTRY,-2);
		sq_rawget(v,-2);
		sq_pushstring(v,module,-1);
		sq_push(v,-4); //repush the closure
		assert(sq_gettype(v,-1) == OT_CLOSURE);
		sq_rawset(v,-3);
		sq_pop(v,2); //pops the registry and module reg
		assert(sq_gettype(v,-1) == OT_CLOSURE);
		assert(sq_gettop(v) == top+1);
		sqstd_fclose(puremodule);
		return SQ_OK;
	}

	//LOAD BINARY////////////////////////////////////////////////////
	scsprintf(path,REL_MODULE_FMT,modulepath,modulename);
	SQBinaryModuleInterface bm;
	bool loaded = false;
	if(SQ_FAILED(api._sqstd_moduleapi_openmodule(v,path,&bm._module))) {
		scsprintf(path,ABS_MODULE_FMT,basepath,modulepath,modulename);
		if(SQ_SUCCEEDED(api._sqstd_moduleapi_openmodule(v,path,&bm._module)))
			loaded = true;
	} else loaded = true;
	if(loaded) {
		sq_pop(v,2); //pops the strings of the path
		if(!(bm._sqstd_module_open = (p_sqstd_module_open)api._sqstd_moduleapi_getsymbol(v,bm._module,_SC("sqstd_module_open")))
			|| !(bm._sqstd_module_close = (p_sqstd_module_close)api._sqstd_moduleapi_getsymbol(v,bm._module,_SC("sqstd_module_close")))
			|| !(bm._sqstd_module_getapiversion = (p_sqstd_module_getapiversion)api._sqstd_moduleapi_getsymbol(v,bm._module,_SC("sqstd_module_getapiversion")))
			|| !(bm._sqstd_module_charsize = (p_sqstd_module_charsize)api._sqstd_moduleapi_getsymbol(v,bm._module,_SC("sqstd_module_charsize"))))
		{
			sq_settop(v,top);
			return sq_throwerror(v,_SC("missing or more required functions in the module interface"));
		}
		bm._api = api;
        sq_settop(v,top);
		if(bm._sqstd_module_getapiversion()>SQSTD_MODULE_API_VERSION) {
			sq_settop(v,top);
			api._sqstd_moduleapi_closemodule(bm._module);
			return sq_throwerror(v,_SC("unsupported api version"));
		}

		/*if(SQ_FAILED(bm._sqstd_module_open(v,&_module_api))) {
			sq_settop(v,top);
			api._sqstd_moduleapi_closemodule(bm._module);
			return SQ_ERROR;
		}*/
		SQBinaryModuleInterface *imod = (SQBinaryModuleInterface *)sq_newuserdata(v,sizeof(SQBinaryModuleInterface));
		(*imod) = bm;
		sq_setreleasehook(v,-1,_sqstd_module_release_hook);
		sq_newclosure(v,_sqstd_module_init,1);
		
		sq_pushregistrytable(v);
		sq_pushstring(v,SQSTD_MODULE_REGISTRY,-1);
		sq_rawget(v,-2);
		sq_pushstring(v,module,-1);
		sq_push(v,-4); //repush the init() closure
		assert(sq_gettype(v,-1) == OT_NATIVECLOSURE);
		sq_rawset(v,-3);
		sq_pop(v,2); //pops the registry and the mod registry
		assert(sq_gettop(v) == top+1);
		//sq_settop(v,top);
		return SQ_OK;
	}
	sq_settop(v,top);
	return sq_throwerror(v,_SC("cannot load the module"));
}


static int _module_import(HSQUIRRELVM v)
{
	const SQChar *module;
	sq_getstring(v,2,&module);
	
	if(SQ_SUCCEEDED(sqstd_importmodule(v,module))){
		/*if(sq_gettop(v) <= 2)
		{
			sq_push(v,1); //caller
		}else {
			if(sq_gettop(v)>3)
			return sq_throwerror(v,_SC("too many paramters"));
			sq_push(v,3);
		}*/
		
		return 1;
	}
	return SQ_ERROR;
}

static int _module_getmoduleregisty(HSQUIRRELVM v)
{
	sq_pushregistrytable(v);
	sq_pushstring(v,SQSTD_MODULE_REGISTRY,-1);
	if(SQ_SUCCEEDED(sq_rawget(v,-2)))
		return 1;
	return SQ_ERROR;
}

SQUIRREL_API SQRESULT sqstd_register_modulelib(HSQUIRRELVM v,SQSTDUserModuleAPI *api)
{
	int top = sq_gettop(v);
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_blob"),-1);
	if(SQ_FAILED(sq_rawget(v,-2))) {
		sq_settop(v,top);
		return sq_throwerror(v,_SC("the module lib requires the blob lib"));
	}
	sq_pop(v,1);
	sq_pushstring(v,_SC("std_io"),-1);
	if(SQ_FAILED(sq_rawget(v,-2))) {
		sq_settop(v,top);
		return sq_throwerror(v,_SC("the module lib requires the blob lib"));
	}
	sq_pop(v,1);
	const SQChar* path = scgetenv(SQSTD_MODULE_PATHVAR);
	if(path == NULL) {
		path = _SC("");
	}
	sq_pushstring(v,SQSTD_MODULE_PATHSLOT,-1);
	//removes the slash at the end of the path(if any)
	SQInteger plen = (SQInteger)scstrlen(path);
	if(plen && (path[plen-1]=='/' || path[plen-1]=='\\'))
		plen--;

	sq_pushstring(v,path,plen);
	sq_rawset(v,-3);
	sq_pushstring(v,SQSTD_MODULE_REGISTRY,-1);
	sq_newtable(v);
	sq_rawset(v,-3);
	sq_pushstring(v,SQSTD_MODULE_USERAPISLOT,-1);
	if(SQ_FAILED(sq_rawget(v,-2))) {
		sq_pushstring(v,SQSTD_MODULE_USERAPISLOT,-1);
		SQSTDUserModuleAPI *ud = (SQSTDUserModuleAPI *)sq_newuserdata(v,sizeof(SQSTDUserModuleAPI));
		*ud = *api;
		sq_rawset(v,-3);
	}
	else {
		sq_pop(v,1);
	}
	sq_pop(v,1); //pops registry
	sq_pushstring(v,_SC("import"),-1);
	sq_newclosure(v,_module_import,0);
	sq_setparamscheck(v,-2,_SC("tst"));
	sq_rawset(v,-3);
	sq_pushstring(v,_SC("getmoduleregistry"),-1);
	sq_newclosure(v,_module_getmoduleregisty,0);
    sq_setparamscheck(v,1,NULL);
	sq_rawset(v,-3);
	
	assert(sq_gettop(v) == top);
	return SQ_OK;
}
