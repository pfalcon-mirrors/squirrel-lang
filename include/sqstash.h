#ifndef _SQ_STASH_H_
#define _SQ_STASH_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <squirrel.h>
#include <sqbloblib.h>

#define SQ_STASH_API_VERSION 1

typedef SQRESULT (*p_sq_compile)(HSQUIRRELVM,HSQREADFUNC,SQUserPointer,const SQChar*,int);
typedef void (*p_sq_push)(HSQUIRRELVM,int);
typedef void (*p_sq_pop)(HSQUIRRELVM,int);
typedef void (*p_sq_remove)(HSQUIRRELVM,int);
typedef int (*p_sq_gettop)(HSQUIRRELVM);
typedef int (*p_sq_cmp)(HSQUIRRELVM);
typedef SQUserPointer(*p_sq_newuserdata)(HSQUIRRELVM,unsigned int);
typedef void(*p_sq_newtable)(HSQUIRRELVM);
typedef void(*p_sq_newarray)(HSQUIRRELVM,int);
typedef void(*p_sq_newclosure)(HSQUIRRELVM,HSQFUNCTION,unsigned int);
typedef void(*p_sq_pushstring)(HSQUIRRELVM,const SQChar *,int);
typedef void(*p_sq_pushfloat)(HSQUIRRELVM,SQFloat);
typedef void(*p_sq_pushinteger)(HSQUIRRELVM,SQInteger);
typedef void(*p_sq_pushuserpointer)(HSQUIRRELVM,SQUserPointer);
typedef void(*p_sq_pushnull)(HSQUIRRELVM);
typedef SQObjectType(*p_sq_gettype)(HSQUIRRELVM,int);
typedef SQInteger(*p_sq_getsize)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_getstring)(HSQUIRRELVM,int,const SQChar **);
typedef SQRESULT(*p_sq_getinteger)(HSQUIRRELVM,int,SQInteger*);
typedef SQRESULT(*p_sq_getfloat)(HSQUIRRELVM,int,SQFloat*);
typedef SQRESULT(*p_sq_getuserpointer)(HSQUIRRELVM,int,SQUserPointer*);
typedef SQRESULT(*p_sq_getuserdata)(HSQUIRRELVM,int,SQUserPointer*);
typedef void(*p_sq_setreleasehook)(HSQUIRRELVM,int,HSQUSERDATARELEASE);
typedef SQChar *(*p_sq_getscratchpad)(HSQUIRRELVM,int);

/*generic object manipulation*/
typedef void(*p_sq_pushroottable)(HSQUIRRELVM);
typedef SQRESULT(*p_sq_set)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_get)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_createslot)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_arrayappend)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_arraypop)(HSQUIRRELVM v,int idx,int pushval);
typedef SQRESULT(*p_sq_arrayresize)(HSQUIRRELVM v,int idx,int newsize);
typedef SQRESULT(*p_sq_setdelegate)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_getdelegate)(HSQUIRRELVM,int);
typedef SQRESULT(*p_sq_setfreevariable)(HSQUIRRELVM,int,unsigned int);
typedef SQRESULT(*p_sq_next)(HSQUIRRELVM,int);

/*calls*/
typedef SQRESULT(*p_sq_call)(HSQUIRRELVM,int,int,int);
typedef const SQChar* (*p_sq_getlocal)(HSQUIRRELVM,unsigned int,unsigned int);

/*raw object handling*/
typedef SQRESULT(*p_sq_getstackobj)(HSQUIRRELVM,int,HSQOBJECT *);
typedef void(*p_sq_pushobject)(HSQUIRRELVM,HSQOBJECT);
typedef void(*p_sq_addref)(HSQUIRRELVM,HSQOBJECT *);
typedef void(*p_sq_release)(HSQUIRRELVM,HSQOBJECT *);
typedef void(*p_sq_resetobject)(HSQOBJECT *);

typedef SQRESULT(*p_sq_throwerror)(HSQUIRRELVM,const SQChar *);

typedef SQRESULT(*p_sq_writeclosure)(HSQUIRRELVM,HSQWRITEFUNC,SQUserPointer);
typedef SQRESULT(*p_sq_readclosure)(HSQUIRRELVM,HSQREADFUNC,SQUserPointer);

typedef void *(*p_sq_malloc)(unsigned int);
typedef void *(*p_sq_realloc)(void*,unsigned int,unsigned int);
typedef void (*p_sq_free)(void*,unsigned int);

/*blob lib*/
typedef void (*p_sq_blob_createdelegate)(HSQUIRRELVM);
typedef void *(*p_sq_blob_newblob)(HSQUIRRELVM,int,SQUIRREL_MALLOC,SQUIRREL_FREE);
typedef SQRESULT(*p_sq_blob_getblob)(HSQUIRRELVM,int,SQUserPointer*);
typedef SQInteger (*p_sq_blob_getblobsize)(HSQUIRRELVM,int idx);


typedef struct tagSQStashAPI{

	p_sq_compile _sq_compile;
	p_sq_push _sq_push;
	p_sq_pop _sq_pop;
	p_sq_remove _sq_remove;
	p_sq_gettop _sq_gettop;
	p_sq_cmp _sq_cmp;
	p_sq_newuserdata _sq_newuserdata;
	p_sq_newtable _sq_newtable;
	p_sq_newarray _sq_newarray;
	p_sq_newclosure _sq_newclosure;
	p_sq_pushstring _sq_pushstring;
	p_sq_pushfloat _sq_pushfloat;
	p_sq_pushinteger _sq_pushinteger;
	p_sq_pushuserpointer _sq_pushuserpointer;
	p_sq_pushnull _sq_pushnull;
	p_sq_gettype _sq_gettype;
	p_sq_getstring _sq_getstring;
	p_sq_getsize _sq_getsize;
	p_sq_getinteger _sq_getinteger;
	p_sq_getfloat _sq_getfloat;
	p_sq_getuserpointer _sq_getuserpointer;
	p_sq_getuserdata _sq_getuserdata;
	p_sq_setreleasehook _sq_setreleasehook;
	p_sq_getscratchpad _sq_getscratchpad;


/*generic object manipulation*/
	p_sq_pushroottable _sq_pushroottable;
	p_sq_set _sq_set;
	p_sq_get _sq_get;
	p_sq_set _sq_rawset;
	p_sq_get _sq_rawget;
	p_sq_createslot _sq_createslot;
	p_sq_arrayappend _sq_arrayappend;
	p_sq_arraypop _sq_arraypop;
	p_sq_arrayresize _sq_arrayresize;
	p_sq_setdelegate _sq_setdelegate;
	p_sq_getdelegate _sq_getdelegate;
	p_sq_setfreevariable _sq_setfreevariable;
	p_sq_next _sq_next;

/*calls*/
	p_sq_call _sq_call;
	p_sq_getlocal _sq_getlocal;

/*raw object handling*/
	p_sq_getstackobj _sq_getstackobj;
	p_sq_pushobject _sq_pushobject;
	p_sq_addref _sq_addref;
	p_sq_release _sq_release;
	p_sq_resetobject _sq_resetobject;

	p_sq_throwerror _sq_throwerror;

	p_sq_writeclosure _sq_writeclosure;
	p_sq_readclosure _sq_readclosure;
	
	p_sq_malloc _sq_malloc;
	p_sq_realloc _sq_realloc;
	p_sq_free _sq_free;	
/*blob lib */
	p_sq_blob_createdelegate _sq_blob_createdelegate;
	p_sq_blob_newblob _sq_blob_newblob;
	p_sq_blob_getblob _sq_blob_getblob;
	p_sq_blob_getblobsize _sq_blob_getblobsize;


}SQStashAPI;

#ifdef SQ_STASH_HELPERS
#define sq_compile (_sa->_sq_compile)
#define sq_push (_sa->_sq_push)
#define sq_pop (_sa->_sq_pop)
#define sq_remove (_sa->_sq_remove)
#define sq_gettop (_sa->_sq_gettop)
#define sq_equal (_sa->_sq_equal)
#define sq_newuserdata (_sa->_sq_newuserdata)
#define sq_newtable (_sa->_sq_newtable)
#define sq_newarray (_sa->_sq_newarray)
#define sq_newclosure (_sa->_sq_newclosure)
#define sq_pushstring (_sa->_sq_pushstring)
#define sq_pushfloat (_sa->_sq_pushfloat)
#define sq_pushinteger (_sa->_sq_pushinteger)
#define sq_pushuserpointer (_sa->_sq_pushuserpointer)
#define sq_pushnull (_sa->_sq_pushnull)
#define sq_gettype (_sa->_sq_gettype)
#define sq_getstring (_sa->_sq_getstring)
#define sq_getsize (_sa->_sq_getsize)
#define sq_getinteger (_sa->_sq_getinteger)
#define sq_getfloat (_sa->_sq_getfloat)
#define sq_getuserpointer (_sa->_sq_getuserpointer)
#define sq_getuserdata (_sa->_sq_getuserdata)
#define sq_setreleasehook (_sa->_sq_setreleasehook)
#define sq_getscratchpad (_sa->_sq_getscratchpad)

#define sq_pushroottable (_sa->_sq_pushroottable)
#define sq_set (_sa->_sq_set)
#define sq_get (_sa->_sq_get)
#define sq_rawset (_sa->_sq_rawset)
#define sq_rawget (_sa->_sq_rawget)
#define sq_createslot (_sa->_sq_createslot)
#define sq_arrayappend (_sa->_sq_arrayappend)
#define sq_arraypop (_sa->_sq_arraypop);
#define sq_arrayresize (_sa->_sq_arrayresize)
#define sq_setdelegate (_sa->_sq_setdelegate)
#define sq_getdelegate (_sa->_sq_getdelegate)
#define sq_setfreevariable (_sa->_sq_setfreevariable)
#define sq_next (_sa->sq_next)
#define sq_call (_sa->_sq_call)
#define sq_getlocal (_sa->_sq_getlocal)
#define sq_getstackobj (_sa->_sq_getstackobj)
#define sq_pushobject (_sa->_sq_pushobject)
#define sq_addref (_sa->_sq_addref)
#define sq_release (_sa->_sq_release)
#define sq_resetobject (_sa->_sq_resetobject)
#define sq_throwerror (_sa->_sq_throwerror)
#define sq_writeclosure (_sa->_sq_writeclosure)
#define sq_readclosure (_sa->_sq_readclosure)
#define sq_malloc (_sa->_sq_malloc)
#define sq_realloc (_sa->_sq_realloc)
#define sq_free (_sa->_sq_free)
#define sq_blob_createdelegate (_sa->_sq_blob_createdelegate)
#define sq_blob_newblob (_sa->_sq_blob_newblob)
#define sq_blob_getblob (_sa->_sq_blob_getblob)
#define sq_blob_getblobsize (_sa->_sq_blob_getblobsize)
#endif //SQ_STASH_HELPERS
/*

//push in the api version,and all supported modules names
int sq_stash_getapiversion(); 

//register the module in the vm, in the pos -1 of the stack there is the table that will host the functions
int sq_stash_open(HSQUIRRELVM,SQStashAPI *)

//close the stash
int sq_stash_close(HSQUIRRELVM v); 

*/

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
