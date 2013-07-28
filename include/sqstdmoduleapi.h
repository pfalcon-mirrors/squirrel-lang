/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_MODULEAPI_H_
#define _SQSTD_MODULEAPI_H_

#include <squirrel.h>
#include <sqstdblob.h>
#include <sqstdio.h>

#define SQSTD_MODULE_API_VERSION 1

/*vm*/
typedef HSQUIRRELVM (*p_sq_open)(int initialstacksize);
typedef HSQUIRRELVM (*p_sq_newthread)(HSQUIRRELVM friendvm, int initialstacksize);
typedef void (*p_sq_seterrorhandler)(HSQUIRRELVM v);
typedef void (*p_sq_close)(HSQUIRRELVM v);
typedef void (*p_sq_setforeignptr)(HSQUIRRELVM v,SQUserPointer p);
typedef SQUserPointer (*p_sq_getforeignptr)(HSQUIRRELVM v);
typedef void (*p_sq_setprintfunc)(HSQUIRRELVM v, SQPRINTFUNCTION printfunc);
typedef SQRESULT (*p_sq_suspendvm)(HSQUIRRELVM v);
typedef SQRESULT (*p_sq_wakeupvm)(HSQUIRRELVM v,int resumedret,int retval);
typedef int (*p_sq_getvmstate)(HSQUIRRELVM v);

/*compiler*/
typedef SQRESULT (*p_sq_compile)(HSQUIRRELVM v,SQLEXREADFUNC read,SQUserPointer p,const SQChar *sourcename,int raiseerror);
typedef SQRESULT (*p_sq_compilebuffer)(HSQUIRRELVM v,const SQChar *s,int size,const SQChar *sourcename,int raiseerror);
typedef void (*p_sq_enabledebuginfo)(HSQUIRRELVM v, int debuginfo);
typedef void (*p_sq_setcompilererrorhandler)(HSQUIRRELVM v,SQCOMPILERERROR f);

/*stack operations*/
typedef void (*p_sq_push)(HSQUIRRELVM v,int idx);
typedef void (*p_sq_pop)(HSQUIRRELVM v,int nelemstopop);
typedef void (*p_sq_remove)(HSQUIRRELVM v,int idx);
typedef int (*p_sq_gettop)(HSQUIRRELVM v);
typedef void (*p_sq_settop)(HSQUIRRELVM v,int newtop);
typedef void (*p_sq_reservestack)(HSQUIRRELVM v,int nsize);
typedef int (*p_sq_cmp)(HSQUIRRELVM v);
typedef void (*p_sq_move)(HSQUIRRELVM dest,HSQUIRRELVM src,int idx);

/*object creation handling*/
typedef SQUserPointer (*p_sq_newuserdata)(HSQUIRRELVM v,unsigned int size);
typedef void (*p_sq_newtable)(HSQUIRRELVM v);
typedef void (*p_sq_newarray)(HSQUIRRELVM v,int size);
typedef void (*p_sq_newclosure)(HSQUIRRELVM v,SQFUNCTION func,unsigned int nfreevars);
typedef SQRESULT (*p_sq_setparamscheck)(HSQUIRRELVM v,int nparamscheck,const SQChar *typemask);
typedef void (*p_sq_pushstring)(HSQUIRRELVM v,const SQChar *s,int len);
typedef void (*p_sq_pushfloat)(HSQUIRRELVM v,SQFloat f);
typedef void (*p_sq_pushinteger)(HSQUIRRELVM v,SQInteger n);
typedef void (*p_sq_pushuserpointer)(HSQUIRRELVM v,SQUserPointer p);
typedef void (*p_sq_pushnull)(HSQUIRRELVM v);
typedef SQObjectType (*p_sq_gettype)(HSQUIRRELVM v,int idx);
typedef SQInteger (*p_sq_getsize)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_getstring)(HSQUIRRELVM v,int idx,const SQChar **c);
typedef SQRESULT (*p_sq_getinteger)(HSQUIRRELVM v,int idx,SQInteger *i);
typedef SQRESULT (*p_sq_getfloat)(HSQUIRRELVM v,int idx,SQFloat *f);
typedef SQRESULT (*p_sq_getuserpointer)(HSQUIRRELVM v,int idx,SQUserPointer *p);
typedef SQRESULT (*p_sq_getuserdata)(HSQUIRRELVM v,int idx,SQUserPointer *p,unsigned int *typetag);
typedef SQRESULT (*p_sq_settypetag)(HSQUIRRELVM v,int idx,unsigned int typetag);
typedef void (*p_sq_setreleasehook)(HSQUIRRELVM v,int idx,SQUSERDATARELEASE hook);
typedef SQChar *(*p_sq_getscratchpad)(HSQUIRRELVM v,int minsize);

/*object manipulation*/
typedef void (*p_sq_pushroottable)(HSQUIRRELVM v);
typedef void (*p_sq_pushregistrytable)(HSQUIRRELVM v);
typedef SQRESULT (*p_sq_setroottable)(HSQUIRRELVM v);
typedef SQRESULT (*p_sq_createslot)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_deleteslot)(HSQUIRRELVM v,int idx,int pushval);
typedef SQRESULT (*p_sq_set)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_get)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_rawget)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_rawset)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_arrayappend)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_arraypop)(HSQUIRRELVM v,int idx,int pushval); 
typedef SQRESULT (*p_sq_arrayresize)(HSQUIRRELVM v,int idx,int newsize); 
typedef SQRESULT (*p_sq_arrayreverse)(HSQUIRRELVM v,int idx); 
typedef SQRESULT (*p_sq_setdelegate)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_getdelegate)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_clone)(HSQUIRRELVM v,int idx);
typedef SQRESULT (*p_sq_setfreevariable)(HSQUIRRELVM v,int idx,unsigned int nval);
typedef SQRESULT (*p_sq_next)(HSQUIRRELVM v,int idx);

/*calls*/
typedef SQRESULT (*p_sq_call)(HSQUIRRELVM v,int params,int retval);
typedef SQRESULT (*p_sq_resume)(HSQUIRRELVM v,int retval);
typedef const SQChar *(*p_sq_getlocal)(HSQUIRRELVM v,unsigned int level,unsigned int idx);
typedef SQRESULT (*p_sq_throwerror)(HSQUIRRELVM v,const SQChar *err);
typedef void (*p_sq_getlasterror)(HSQUIRRELVM v);

/*raw object handling*/
typedef SQRESULT (*p_sq_getstackobj)(HSQUIRRELVM v,int idx,HSQOBJECT *po);
typedef void (*p_sq_pushobject)(HSQUIRRELVM v,HSQOBJECT obj);
typedef void (*p_sq_addref)(HSQUIRRELVM v,HSQOBJECT *po);
typedef void (*p_sq_release)(HSQUIRRELVM v,HSQOBJECT *po);
typedef void (*p_sq_resetobject)(HSQOBJECT *po);

/*GC*/
typedef int (*p_sq_collectgarbage)(HSQUIRRELVM v);

/*serialization*/
typedef SQRESULT (*p_sq_writeclosure)(HSQUIRRELVM vm,SQWRITEFUNC writef,SQUserPointer up);
typedef SQRESULT (*p_sq_readclosure)(HSQUIRRELVM vm,SQREADFUNC readf,SQUserPointer up);

typedef void *(*p_sq_malloc)(unsigned int size);
typedef void *(*p_sq_realloc)(void* p,unsigned int oldsize,unsigned int newsize);
typedef void (*p_sq_free)(void *p,unsigned int size);

/*debug*/
typedef SQRESULT (*p_sq_stackinfos)(HSQUIRRELVM v,int level,SQStackInfos *si);
typedef void (*p_sq_setdebughook)(HSQUIRRELVM v);

/*stdlib io*/
typedef SQFILE (*p_sqstd_fopen)(const SQChar *,const SQChar *);
typedef SQInteger (*p_sqstd_fread)(SQUserPointer, SQInteger, SQInteger, SQFILE);
typedef SQInteger (*p_sqstd_fwrite)(const SQUserPointer, SQInteger, SQInteger, SQFILE);
typedef int (*p_sqstd_fseek)(SQFILE , long , int);
typedef long (*p_sqstd_ftell)(SQFILE);
typedef int (*p_sqstd_fflush)(SQFILE);
typedef int (*p_sqstd_fclose)(SQFILE);
typedef int (*p_sqstd_feof)(SQFILE);

typedef SQRESULT (*p_sqstd_createfile)(HSQUIRRELVM v, SQFILE file,int own);
typedef SQRESULT (*p_sqstd_getfile)(HSQUIRRELVM v, int idx, SQFILE *file);

/*sqstd blob*/
typedef SQUserPointer (*p_sqstd_createblob)(HSQUIRRELVM v, int size);
typedef SQRESULT (*p_sqstd_getblob)(HSQUIRRELVM v,int idx,SQUserPointer *ptr);
typedef int (*p_sqstd_getblobsize)(HSQUIRRELVM v,int idx);


typedef struct tagSQModuleAPI {

	p_sq_open _sq_open;
	p_sq_newthread _sq_newthread;
	p_sq_seterrorhandler _sq_seterrorhandler;
	p_sq_close _sq_close;
	p_sq_setforeignptr _sq_setforeignptr;
	p_sq_getforeignptr _sq_getforeignptr;
	p_sq_setprintfunc _sq_setprintfunc;
	p_sq_suspendvm _sq_suspendvm;
	p_sq_wakeupvm _sq_wakeupvm;
	p_sq_getvmstate _sq_getvmstate;

/*compiler*/
	p_sq_compile _sq_compile;
	p_sq_compilebuffer _sq_compilebuffer;
	p_sq_enabledebuginfo _sq_enabledebuginfo;
	p_sq_setcompilererrorhandler _sq_setcompilererrorhandler;

/*stack operations*/
	p_sq_push _sq_push;
	p_sq_pop _sq_pop;
	p_sq_remove _sq_remove;
	p_sq_gettop _sq_gettop;
	p_sq_settop _sq_settop;
	p_sq_reservestack _sq_reservestack;
	p_sq_cmp _sq_cmp;
	p_sq_move _sq_move;

/*object creation handling*/
	p_sq_newuserdata _sq_newuserdata;
	p_sq_newtable _sq_newtable;
	p_sq_newarray _sq_newarray;
	p_sq_newclosure _sq_newclosure;
	p_sq_setparamscheck _sq_setparamscheck;
	p_sq_pushstring _sq_pushstring;
	p_sq_pushfloat _sq_pushfloat;
	p_sq_pushinteger _sq_pushinteger;
	p_sq_pushuserpointer _sq_pushuserpointer;
	p_sq_pushnull _sq_pushnull;
	p_sq_gettype _sq_gettype;
	p_sq_getsize _sq_getsize;
	p_sq_getstring _sq_getstring;
	p_sq_getinteger _sq_getinteger;
	p_sq_getfloat _sq_getfloat;
	p_sq_getuserpointer _sq_getuserpointer;
	p_sq_getuserdata _sq_getuserdata;
	p_sq_settypetag _sq_settypetag;
	p_sq_setreleasehook _sq_setreleasehook;
	p_sq_getscratchpad _sq_getscratchpad;

/*object manipulation*/
	p_sq_pushroottable _sq_pushroottable;
	p_sq_pushregistrytable _sq_pushregistrytable;
	p_sq_setroottable _sq_setroottable;
	p_sq_createslot _sq_createslot;
	p_sq_deleteslot _sq_deleteslot;
	p_sq_set _sq_set;
	p_sq_get _sq_get;
	p_sq_rawget _sq_rawget;
	p_sq_rawset _sq_rawset;
	p_sq_arrayappend _sq_arrayappend;
	p_sq_arraypop _sq_arraypop;
	p_sq_arrayresize _sq_arrayresize;
	p_sq_arrayreverse _sq_arrayreverse;
	p_sq_setdelegate _sq_setdelegate;
	p_sq_getdelegate _sq_getdelegate;
	p_sq_clone _sq_clone;
	p_sq_setfreevariable _sq_setfreevariable;
	p_sq_next _sq_next;

/*calls*/
	p_sq_call _sq_call;
	p_sq_resume _sq_resume;
	p_sq_getlocal _sq_getlocal;
	p_sq_throwerror _sq_throwerror;
	p_sq_getlasterror _sq_getlasterror;

/*raw object handling*/
	p_sq_getstackobj _sq_getstackobj;
	p_sq_pushobject _sq_pushobject;
	p_sq_addref _sq_addref;
	p_sq_release _sq_release;
	p_sq_resetobject _sq_resetobject;

/*GC*/
	p_sq_collectgarbage _sq_collectgarbage;

/*serialization*/
	p_sq_writeclosure _sq_writeclosure;
	p_sq_readclosure _sq_readclosure;

	p_sq_malloc _sq_malloc;
	p_sq_realloc _sq_realloc;
	p_sq_free _sq_free;

/*debug*/
	p_sq_stackinfos _sq_stackinfos;
	p_sq_setdebughook _sq_setdebughook;

/*sqstd io*/
	p_sqstd_fopen _sqstd_fopen;
	p_sqstd_fread _sqstd_fread;
	p_sqstd_fwrite _sqstd_fwrite;
	p_sqstd_fseek _sqstd_fseek;
	p_sqstd_ftell _sqstd_ftell;
	p_sqstd_fflush _sqstd_fflush;
	p_sqstd_fclose _sqstd_fclose;
	p_sqstd_feof _sqstd_feof;

	p_sqstd_createfile _sqstd_createfile;
	p_sqstd_getfile _sqstd_getfile;

/*sqstd blob*/
	p_sqstd_createblob _sqstd_createblob;
	p_sqstd_getblob _sqstd_getblob;
	p_sqstd_getblobsize _sqstd_getblobsize;

	

} SQModuleAPI;

#ifdef SQSTD_MODULE_HELPERS

#define _SQSTD_DECLARE_MODULE_API() \
	SQModuleAPI *_mapi = NULL;

#define _SQSTD_INIT_MODULE_API(module_api) \
	_mapi = module_api;

#define sq_open (_mapi->_sq_open)
#define sq_newthread (_mapi->_sq_newthread)
#define sq_seterrorhandler (_mapi->_sq_seterrorhandler)
#define sq_close (_mapi->_sq_close)
#define sq_setforeignptr (_mapi->_sq_setforeignptr)
#define sq_getforeignptr (_mapi->_sq_getforeignptr)
#define sq_setprintfunc (_mapi->_sq_setprintfunc)
#define sq_suspendvm (_mapi->_sq_suspendvm)
#define sq_wakeupvm (_mapi->_sq_wakeupvm)
#define sq_getvmstate (_mapi->_sq_getvmstate)

/*compiler*/
#define sq_compile (_mapi->_sq_compile)
#define sq_compilebuffer (_mapi->_sq_compilebuffer)
#define sq_enabledebuginfo (_mapi->_sq_enabledebuginfo)
#define sq_setcompilererrorhandler (_mapi->_sq_setcompilererrorhandler)

/*stack operations*/
#define sq_push (_mapi->_sq_push)
#define sq_pop (_mapi->_sq_pop)
#define sq_remove (_mapi->_sq_remove)
#define sq_gettop (_mapi->_sq_gettop)
#define sq_settop (_mapi->_sq_settop)
#define sq_reservestack (_mapi->_sq_reservestack)
#define sq_cmp (_mapi->_sq_cmp)
#define sq_move (_mapi->_sq_move)

/*object creation handling*/
#define sq_newuserdata (_mapi->_sq_newuserdata)
#define sq_newtable (_mapi->_sq_newtable)
#define sq_newarray (_mapi->_sq_newarray)
#define sq_newclosure (_mapi->_sq_newclosure)
#define sq_setparamscheck (_mapi->_sq_setparamscheck)
#define sq_pushstring (_mapi->_sq_pushstring)
#define sq_pushfloat (_mapi->_sq_pushfloat)
#define sq_pushinteger (_mapi->_sq_pushinteger)
#define sq_pushuserpointer (_mapi->_sq_pushuserpointer)
#define sq_pushnull (_mapi->_sq_pushnull)
#define sq_gettype (_mapi->_sq_gettype)
#define sq_getsize (_mapi->_sq_getsize)
#define sq_getstring (_mapi->_sq_getstring)
#define sq_getinteger (_mapi->_sq_getinteger)
#define sq_getfloat (_mapi->_sq_getfloat)
#define sq_getuserpointer (_mapi->_sq_getuserpointer)
#define sq_getuserdata (_mapi->_sq_getuserdata)
#define sq_settypetag (_mapi->_sq_settypetag)
#define sq_setreleasehook (_mapi->_sq_setreleasehook)
#define sq_getscratchpad (_mapi->_sq_getscratchpad)

/*object manipulation*/
#define sq_pushroottable (_mapi->_sq_pushroottable)
#define sq_pushregistrytable (_mapi->_sq_pushregistrytable)
#define sq_setroottable (_mapi->_sq_setroottable)
#define sq_createslot (_mapi->_sq_createslot)
#define sq_deleteslot (_mapi->_sq_deleteslot)
#define sq_set (_mapi->_sq_set)
#define sq_get (_mapi->_sq_get)
#define sq_rawget (_mapi->_sq_rawget)
#define sq_rawset (_mapi->_sq_rawset)
#define sq_arrayappend (_mapi->_sq_arrayappend)
#define sq_arraypop (_mapi->_sq_arraypop)
#define sq_arrayresize (_mapi->_sq_arrayresize)
#define sq_arrayreverse (_mapi->_sq_arrayreverse)
#define sq_setdelegate (_mapi->_sq_setdelegate)
#define sq_getdelegate (_mapi->_sq_getdelegate)
#define sq_clone (_mapi->_sq_clone)
#define sq_setfreevariable (_mapi->_sq_setfreevariable)
#define sq_next (_mapi->_sq_next)

/*calls*/
#define sq_call (_mapi->_sq_call)
#define sq_resume (_mapi->_sq_resume)
#define sq_getlocal (_mapi->_sq_getlocal)
#define sq_throwerror (_mapi->_sq_throwerror)
#define sq_getlasterror (_mapi->_sq_getlasterror)

/*raw object handling*/
#define sq_getstackobj (_mapi->_sq_getstackobj)
#define sq_pushobject (_mapi->_sq_pushobject)
#define sq_addref (_mapi->_sq_addref)
#define sq_release (_mapi->_sq_release)
#define sq_resetobject (_mapi->_sq_resetobject)

/*GC*/
#define sq_collectgarbage (_mapi->_sq_collectgarbage)

/*serialization*/
#define sq_writeclosure (_mapi->_sq_writeclosure)
#define sq_readclosure (_mapi->_sq_readclosure)

#define sq_malloc (_mapi->_sq_malloc)
#define sq_realloc (_mapi->_sq_realloc)
#define sq_free (_mapi->_sq_free)

/*debug*/
#define sq_stackinfos (_mapi->_sq_stackinfos)
#define sq_setdebughook (_mapi->_sq_setdebughook)

/*sqstd io*/
#define sqstd_fopen (_mapi->_sqstd_fopen)
#define sqstd_fread (_mapi->_sqstd_fread)
#define sqstd_fwrite (_mapi->_sqstd_fwrite)
#define sqstd_fseek (_mapi->_sqstd_fseek)
#define sqstd_ftell (_mapi->_sqstd_ftell)
#define sqstd_fflush (_mapi->_sqstd_fflush)
#define sqstd_fclose (_mapi->_sqstd_fclose)
#define sqstd_feof (_mapi->_sqstd_feof)

#define sqstd_createfile (_mapi->_sqstd_createfile)
#define sqstd_getfile (_mapi->_sqstd_getfile)

/*sqstd blob*/
#define sqstd_createblob (_mapi->_sqstd_createblob)
#define sqstd_getblob (_mapi->_sqstd_getblob)
#define sqstd_getblobsize (_mapi->_sqstd_getblobsize)

#endif //SQ_MODULE_HELPERS

#endif /*_SQSTD_MODULEAPI_H_*/
