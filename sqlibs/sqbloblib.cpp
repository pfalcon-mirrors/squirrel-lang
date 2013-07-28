#include <squirrel.h>
#include "sqbloblib.h"
#include <string.h>
#include <new>

static SQUIRREL_MALLOC g_default_malloc=NULL;
static SQUIRREL_FREE g_default_free=NULL;


#define chk_params(_v,_n) \
	if(sq_gettop(_v)!=_n) return sq_throwerror(v,_SC("invalid number of args"));

#define chk_params_at_least(n) \
	if(sq_gettop(_v)<_n) return sq_throwerror(v,_SC("invalid number of args"));

#define SQ_SEEK_SET 's'
#define SQ_SEEK_CUR 'c'
#define SQ_SEEK_END 'e'

SQFloat sq_aux_getfloat(HSQUIRRELVM v,int idx)
{
	SQFloat f=0;
	sq_getfloat(v,idx,&f);
	return f;
}

SQInteger sq_aux_getinteger(HSQUIRRELVM v,int idx)
{
	SQInteger i=0;
	sq_getinteger(v,idx,&i);
	return i;
}

struct _Blob
{
	_Blob(int size,SQUIRREL_MALLOC __malloc__,SQUIRREL_FREE __free__)
	{
		_size=size;
		_malloc=__malloc__;
		_free=__free__;
		_buf=(unsigned char *)_malloc(size);
		memset(_buf,0,_size);
		_ptr=0;
	}
	~_Blob()
	{
		_free(_buf,_size);
	}
	bool Write(void *p,int n)
	{
		if(!CanAdvance(n))return false;
		memcpy(&_buf[_ptr],p,n);_ptr+=n;
		return true;
	}
	bool Read(void *p,int n)
	{
		if(!CanAdvance(n))return false;
		memcpy(p,&_buf[_ptr],n);_ptr+=n;
		return true;
	}
	void Resize(int n)
	{
		if(n!=_size)
		{
			unsigned char *newbuf=(unsigned char *)_malloc(n);
			memset(newbuf,0,n);
			if(_size>n)
				memcpy(newbuf,_buf,n);
			else
				memcpy(newbuf,_buf,_size);
			_free(_buf,_size);
			_size=n;
			_buf=newbuf;
		}
	}
	bool CanAdvance(int n)
	{
		if(_ptr+n>_size)return false;
		return true;
	}
	int Seek(int mode,int n){
		switch(mode){
			case SQ_SEEK_SET:
				if(_ptr>=_size || _ptr<0)return false;
				_ptr=n;
				break;
			case SQ_SEEK_CUR:
				if(_ptr+n>=_size || _ptr+n<0)return false;
				_ptr+=n;
				break;
			case SQ_SEEK_END:
				if(_size+n>=_size || _size+n<0)return false;
				_ptr=_size+n;
				break;
			default:return false;
		}
		return true;
	}
	
	int _size;
	int _ptr;
	unsigned char *_buf;
	SQUIRREL_MALLOC _malloc;
	SQUIRREL_FREE _free;
};

#define BLOB_MAGIC_NUMBER 0XB10BB10B

struct Blob_Userdata
{
	Blob_Userdata(int size,SQUIRREL_MALLOC __malloc,SQUIRREL_FREE __free):_blob(size,__malloc,__free)
	{
		_magic_number=BLOB_MAGIC_NUMBER;
	}
	
	unsigned int _magic_number;
	_Blob _blob;
};


#define blob_readX(v,type) \
	Blob_Userdata *bu; \
	if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&bu)) || bu->_magic_number!=BLOB_MAGIC_NUMBER)return 0; \
	_Blob *b=&(bu->_blob); \
	type t; \
	if(!b->Read(&t,sizeof(t)))return 0;

#define blob_read_X(v,type) \
	type t; \
	if(!_blob_read_void(v,sizeof(type),&t)) return 0; 

int _blob_read_void(HSQUIRRELVM v,int size,void *dest)
{
	chk_params(v,1);
	Blob_Userdata *bu;
	if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&bu)) || bu->_magic_number!=BLOB_MAGIC_NUMBER)return 0;
	_Blob *b=&bu->_blob;
	if(!b)return 0;
	int sdiff=4-size;
	if(!b->Read(dest,size))return 0;
	return 1;
}

#define blob_writeX(v,type,func) \
	chk_params(v,2); \
	Blob_Userdata *bu; \
	if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&bu)) || bu->_magic_number!=BLOB_MAGIC_NUMBER)return 0; \
	_Blob *b=&bu->_blob; \
	type t=func(v,2); \
	if(!b->Write(&t,sizeof(t)))return sq_throwerror(v,_SC("end of blob"));


#define GET_BLOB(idx) Blob_Userdata *bu; \
	if(SQ_FAILED(sq_getuserdata(v,idx,(SQUserPointer*)&bu)) || bu->_magic_number!=BLOB_MAGIC_NUMBER)return 0; \
	_Blob *b=&bu->_blob; 

int blob_readI1(HSQUIRRELVM v)
{
	blob_read_X(v,char)
	sq_pushinteger(v,t);
	return 1;
}

int blob_readI2(HSQUIRRELVM v)
{
	blob_read_X(v,short);	
	sq_pushinteger(v,t);
	return 1;
}

int blob_readU1(HSQUIRRELVM v)
{
	blob_read_X(v,unsigned char)
	sq_pushinteger(v,t);
	return 1;
}

int blob_readU2(HSQUIRRELVM v)
{
	blob_read_X(v,unsigned short);	
	sq_pushinteger(v,t);
	return 1;
}

int blob_readI4(HSQUIRRELVM v)
{
	blob_read_X(v,int);	
	sq_pushinteger(v,t);
	return 1;
}

int blob_readF4(HSQUIRRELVM v)
{
	blob_read_X(v,float);	
	sq_pushfloat(v,t);
	return 1;
}

int blob_readF8(HSQUIRRELVM v)
{
	blob_read_X(v,double);	
	sq_pushfloat(v,(SQFloat)t);
	return 1;
}

int blob_seek(HSQUIRRELVM v)
{
	chk_params(v,3);
	return 0;
}

int blob_writeI1(HSQUIRRELVM v)
{
	blob_writeX(v,char,sq_aux_getinteger);
	return 0;
}

int blob_writeI2(HSQUIRRELVM v)
{
	blob_writeX(v,short,sq_aux_getinteger);
	return 0;
}

int blob_writeI4(HSQUIRRELVM v)
{
	blob_writeX(v,int,sq_aux_getinteger);
	return 0;
}

int blob_writeU2(HSQUIRRELVM v)
{
	blob_writeX(v,unsigned short,sq_aux_getinteger);
	return 0;
}

int blob_writeU1(HSQUIRRELVM v)
{
	blob_writeX(v,unsigned char,sq_aux_getinteger);
	return 0;
}

int blob_writeF4(HSQUIRRELVM v)
{
	blob_writeX(v,float,sq_aux_getfloat);
	return 0;
}

int blob_writeF8(HSQUIRRELVM v)
{
	blob_writeX(v,double,sq_aux_getfloat);
	return 0;
}

int blob_readblob(HSQUIRRELVM v)
{
	chk_params(v,2);
	GET_BLOB(1)
	int size;sq_getinteger(v,2,&size);
	if(size>0 && b->CanAdvance(size))
	{
		void *newblob=sq_blob_newblob(v,size,NULL,NULL);
		b->Read(newblob,size);
		return 1;
	}
	return sq_throwerror(v,_SC("cannot read"));
}

void __swap_dword(unsigned int *n)
{
		*n=(unsigned int)(((*n&0xFF000000)>>24)  |
				((*n&0x00FF0000)>>8)  |
				((*n&0x0000FF00)<<8)  |
				((*n&0x000000FF)<<24));
}

int blob_swapblob4(HSQUIRRELVM v)
{
	GET_BLOB(1)
	int num=(b->_size-(b->_size%4))>>2;
	unsigned int *t=(unsigned int *)b->_buf;
	for(int i=0;i<num;i++)
	{
		__swap_dword(&t[i]);
	}
	return 0;
}

int blob_swapblob2(HSQUIRRELVM v)
{
	return 0;
}

int blob_cloneblob(HSQUIRRELVM v)
{
	chk_params(v,1);
	SQUserPointer blob;
	if(SQ_FAILED(sq_blob_getblob(v,1,&blob)))return 0;
	int size=sq_blob_getblobsize(v,1);
	SQUserPointer newblob=sq_blob_newblob(v,size,NULL,NULL);
	memcpy(newblob,blob,size);
	return 1;
}

int blob_size(HSQUIRRELVM v)
{
	GET_BLOB(1)
	sq_pushinteger(v,b->_size);
	return 1;
}

int blob_tell(HSQUIRRELVM v)
{
	GET_BLOB(1)
	sq_pushinteger(v,b->_ptr);
	return 1;
}

int blob_resize(HSQUIRRELVM v)
{
	GET_BLOB(1)
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("size must be anumber"));
	int newsize=sq_aux_getinteger(v,2);
	if(newsize<=0)return sq_throwerror(v,_SC("size must >= 0"));
	b->Resize(newsize);
	return 0;
}

int blob__set(HSQUIRRELVM v)
{
	GET_BLOB(1)
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("the index is not a number"));
	if(!(sq_gettype(v,3)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("the value is not a number"));
	SQInteger idx=sq_aux_getinteger(v,2);
	SQInteger val=sq_aux_getinteger(v,3);
	if(idx<0 || idx>=b->_size)return sq_throwerror(v,_SC("index out of range"));
	b->_buf[idx]=val;
	sq_pushinteger(v,val);
	return 1;
}

int blob__get(HSQUIRRELVM v)
{
	GET_BLOB(1)
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("the index is not a number"));
	SQInteger idx=sq_aux_getinteger(v,2);
	if(idx<0 || idx>=b->_size)return sq_throwerror(v,_SC("index out of range"));
	sq_pushinteger(v,b->_buf[idx]);
	return 1;
}

int blob__typeof(HSQUIRRELVM v)
{
	sq_pushstring(v,_SC("blob"),-1);
	return 1;
}

int blob__nexti(HSQUIRRELVM v)
{
	chk_params(v,2);
	GET_BLOB(1)
	if(sq_gettype(v,2)==OT_NULL)
	{
		sq_pushinteger(v,0);
		return 1;
	}
	SQInteger idx=sq_aux_getinteger(v,2);
	if(idx+1<b->_size)
	{
		sq_pushinteger(v,idx+1);
		return 1;
	}
	sq_pushnull(v);
	return 1;
}

//globals
int blob_rawcastI2F(HSQUIRRELVM v)
{
	chk_params(v,2);
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("number expected"));
	SQInteger i=sq_aux_getinteger(v,2);
	sq_pushfloat(v,*((SQFloat *)&i));
	return 1;
}

int blob_rawcastF2I(HSQUIRRELVM v)
{
	chk_params(v,2);
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("number expected"));
	SQFloat f=sq_aux_getfloat(v,2);
	sq_pushinteger(v,*((SQInteger *)&f));
	return 1;
}

int blob_swap2(HSQUIRRELVM v)
{
	chk_params(v,2);
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("number expected"));
	short s=sq_aux_getinteger(v,2);
	short res=(s<<8)|((s>>8)&0x00FF);
	sq_pushinteger(v,res);
	return 1;
	
}



int blob_swap4(HSQUIRRELVM v)
{
	chk_params(v,2);
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("number expected"));
	SQInteger i=sq_aux_getinteger(v,2);
	__swap_dword((unsigned int *)&i);
	sq_pushinteger(v,i);
	return 1;
}

int blob_swapfloat(HSQUIRRELVM v)
{
	chk_params(v,2);
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("number expected"));
	SQFloat f=sq_aux_getfloat(v,2);
	__swap_dword((unsigned int *)&f);
	sq_pushfloat(v,f);
	return 1;
}

int blob_eob(HSQUIRRELVM v)
{
	GET_BLOB(1);
	if(!b->CanAdvance(1))
	{
		sq_pushinteger(v,1);
		return 1;
	}
	return 0;
}


#define _DECL_FUNC(name,nparams) {_SC(#name),blob_##name,nparams}
static SQRegFunction blob_funcs[]={
	_DECL_FUNC(eob,1),
	_DECL_FUNC(readI1,1),
    _DECL_FUNC(readI2,1),
    _DECL_FUNC(readI4,1),
	_DECL_FUNC(readU2,1),
    _DECL_FUNC(readU1,1),
    _DECL_FUNC(readF4,1),
	_DECL_FUNC(readF8,1),
	_DECL_FUNC(writeI1,2),
    _DECL_FUNC(writeI2,2),
    _DECL_FUNC(writeI4,2),
	_DECL_FUNC(writeU2,2),
    _DECL_FUNC(writeU1,2),
    _DECL_FUNC(writeF4,2),
	_DECL_FUNC(writeF8,2),
	_DECL_FUNC(swapblob4,1),
	_DECL_FUNC(swapblob2,1),
    _DECL_FUNC(seek,-2),
    _DECL_FUNC(readblob,2),
    _DECL_FUNC(cloneblob,1),
	_DECL_FUNC(size,1),
	_DECL_FUNC(tell,1),
	_DECL_FUNC(resize,2),
// reflex
	_DECL_FUNC(_set,3),
	_DECL_FUNC(_get,2),
	_DECL_FUNC(_typeof,1),
	_DECL_FUNC(_nexti,2),
	{0,0}
};

static SQRegFunction bloblib_funcs[]={
	_DECL_FUNC(rawcastI2F,2),
	_DECL_FUNC(rawcastF2I,2),
	_DECL_FUNC(swap2,2),
	_DECL_FUNC(swap4,2),
	_DECL_FUNC(swapfloat,2),

	{0,0}
};

//globals
int blob_crateblob(HSQUIRRELVM v)
{
	int n=sq_gettop(v);
	chk_params(v,3); //1 is the outer value
	if(!(sq_gettype(v,2)&SQOBJECT_NUMERIC))return sq_throwerror(v,_SC("the size is not a number"));
	SQInteger size=sq_aux_getinteger(v,2);
	sq_blob_newblob(v,size,NULL,NULL);
	return 1;
}

//library
void sq_blob_createdelegate(HSQUIRRELVM v)
{
	int i=0;
	i=0;
	sq_pushroottable(v);
	sq_pushstring(v,_SC("__sq_blob_delegate__"),-1);
	if(SQ_FAILED(sq_get(v,-2)))
	{
		sq_pop(v,1);

		sq_newtable(v);
		////functions
		while(blob_funcs[i].name!=0)
		{
			sq_pushstring(v,blob_funcs[i].name,-1);
			sq_newclosure(v,blob_funcs[i].f,blob_funcs[i].nparamscheck,0);
			sq_createslot(v,-3);
			i++;
		}

		sq_pushroottable(v);
		sq_pushstring(v,_SC("__sq_blob_delegate__"),-1);
		sq_push(v,-3);
		sq_createslot(v,-3);
		sq_pop(v,1);
		return;
	}
	sq_remove(v,-2);
	
}

int _blob_release(SQUserPointer p,int size)
{
	Blob_Userdata *bu=(Blob_Userdata *)p;
	if(bu->_magic_number==BLOB_MAGIC_NUMBER)
	{
		bu->_blob.~_Blob();
	}
	return 1;
}

void *sq_blob_newblob(HSQUIRRELVM v,int size,SQUIRREL_MALLOC __malloc,SQUIRREL_FREE __free)
{
	if(!__malloc)__malloc=g_default_malloc;
	if(!__free)__free=g_default_free;
	Blob_Userdata *bu=(Blob_Userdata *)sq_newuserdata(v,sizeof(Blob_Userdata));
	new (bu) Blob_Userdata(size,__malloc,__free);
	sq_blob_createdelegate(v);
	sq_setdelegate(v,-2);
	sq_setreleasehook(v,-1,_blob_release);
	return bu->_blob._buf;
}

SQRESULT sq_blob_getblob(HSQUIRRELVM v,int idx,SQUserPointer *p)
{
	GET_BLOB(idx)
	if(!b)return SQ_ERROR;
	*p=b->_buf;
	return SQ_OK;
}

SQInteger sq_blob_getblobsize(HSQUIRRELVM v,int idx)
{
	GET_BLOB(idx)
	if(!b)return -1;
	return b->_size;
}

void sq_blob_register(HSQUIRRELVM v,SQUIRREL_MALLOC _default_malloc,SQUIRREL_FREE _default_free)
{
	int i=0;
	g_default_malloc=_default_malloc;
	g_default_free=_default_free;
	while(bloblib_funcs[i].name!=0)
	{
		sq_pushstring(v,bloblib_funcs[i].name,-1);
		sq_newclosure(v,bloblib_funcs[i].f,bloblib_funcs[i].nparamscheck,0);
		sq_createslot(v,-3);
		i++;
	}
	sq_pushstring(v,_SC("createblob"),-1);
	sq_blob_createdelegate(v);
	sq_newclosure(v,blob_crateblob,2,1);
	sq_createslot(v,-3);
}


