/* see copyright notice in squirrel.h */
#include <new>
#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>
#include "sqstdblob.h"
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

#define SQSTD_BLOB_TYPE_TAG (SQSTD_STREAM_TYPE_TAG | 0x00000002)

//Blob


#define SETUP_BLOB(v) \
	SQBlob *self = NULL; \
	sq_getuserdata(v,1,(SQUserPointer *)&self,NULL); 


static int _blob_resize(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger size;
	sq_getinteger(v,2,&size);
	if(!self->Resize(size))
		return sq_throwerror(v,_SC("resize failed"));
	return 0;
}

static void __swap_dword(unsigned int *n)
{
	*n=(unsigned int)(((*n&0xFF000000)>>24)  |
			((*n&0x00FF0000)>>8)  |
			((*n&0x0000FF00)<<8)  |
			((*n&0x000000FF)<<24));
}

static void __swap_word(unsigned short *n)
{
	*n=(unsigned short)((*n>>8)&0x00FF)| ((*n<<8)&0xFF00);
}

static int _blob_swap4(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	int num=(self->Len()-(self->Len()%4))>>2;
	unsigned int *t=(unsigned int *)self->GetBuf();
	for(int i = 0; i < num; i++) {
		__swap_dword(&t[i]);
	}
	return 0;
}

static int _blob_swap2(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	int num=(self->Len()-(self->Len()%2))>>1;
	unsigned short *t = (unsigned short *)self->GetBuf();
	for(int i = 0; i < num; i++) {
		__swap_word(&t[i]);
	}
	return 0;
}

static int _blob__set(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger idx,val;
	sq_getinteger(v,2,&idx);
	sq_getinteger(v,3,&val);
	if(idx < 0 || idx >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));
	((unsigned char *)self->GetBuf())[idx] = (unsigned char) val;
	sq_push(v,3);
	return 1;
}

static int _blob__get(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	SQInteger idx;
	sq_getinteger(v,2,&idx);
	if(idx < 0 || idx >= self->Len())
		return sq_throwerror(v,_SC("index out of range"));
	sq_pushinteger(v,((unsigned char *)self->GetBuf())[idx]);
	return 1;
}

static int _blob__nexti(HSQUIRRELVM v)
{
	SETUP_BLOB(v);
	if(sq_gettype(v,2) == OT_NULL) {
		sq_pushinteger(v, 0);
		return 1;
	}
	SQInteger idx;
	if(SQ_SUCCEEDED(sq_getinteger(v, 2, &idx))) {
		if(idx+1 < self->Len()) {
			sq_pushinteger(v, idx+1);
			return 1;
		}
		sq_pushnull(v);
		return 1;
	}
	return sq_throwerror(v,_SC("internal error (_nexti) wrong argument type"));
}

static int _blob__typeof(HSQUIRRELVM v)
{
	sq_pushstring(v,_SC("blob"),-1);
	return 1;
}

#define _DECL_BLOB_FUNC(name,nparams,typecheck) {_SC(#name),_blob_##name,nparams,typecheck}
static SQRegFunction _blob_delegate[] = {
	_DECL_STREAM_FUNC(readstr,-2,_SC("unn")),
	_DECL_STREAM_FUNC(readblob,2,_SC("un")),
	_DECL_STREAM_FUNC(readn,2,_SC("un")),
	_DECL_STREAM_FUNC(writestr,-2,_SC("usn")),
	_DECL_STREAM_FUNC(writeblob,-2,_SC("uu")),
	_DECL_STREAM_FUNC(writen,3,_SC("unn")),
	_DECL_STREAM_FUNC(seek,-2,_SC("unn")),
	_DECL_STREAM_FUNC(tell,1,_SC("u")),
	_DECL_STREAM_FUNC(len,1,_SC("u")),
	_DECL_STREAM_FUNC(eos,1,_SC("u")),
	_DECL_STREAM_FUNC(flush,1,_SC("u")),
	_DECL_BLOB_FUNC(resize,2,_SC("un")),
	_DECL_BLOB_FUNC(swap2,1,_SC("u")),
	_DECL_BLOB_FUNC(swap4,1,_SC("u")),
	_DECL_BLOB_FUNC(_set,3,_SC("unn")),
	_DECL_BLOB_FUNC(_get,2,_SC("un")),
	_DECL_BLOB_FUNC(_typeof,1,_SC("u")),
	_DECL_BLOB_FUNC(_nexti,2,_SC("u")),
	{0,0,0,0}
};

static int _blob_releasehook(SQUserPointer p, int size)
{
	SQBlob *self = (SQBlob *)p;
	self->~SQBlob();
	return 1;
}

//GLOBAL FUNCTIONS

static int _g_blob_blob(HSQUIRRELVM v)
{
	SQInteger size = 0;
	sq_getinteger(v, 2, &size);
	if(size < 0) return sq_throwerror(v, _SC("cannot create blob with negative size"));
	sqstd_createblob(v,size);
	return 1;
}

static int _g_blob_casti2f(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	sq_pushfloat(v,*((SQFloat *)&i));
	return 1;
}

static int _g_blob_castf2i(HSQUIRRELVM v)
{
	SQFloat f;
	sq_getfloat(v,2,&f);
	sq_pushinteger(v,*((SQInteger *)&f));
	return 1;
}

static int _g_blob_swap2(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	short s=(short)i;
	sq_pushinteger(v,(s<<8)|((s>>8)&0x00FF));
	return 1;
}

static int _g_blob_swap4(HSQUIRRELVM v)
{
	SQInteger i;
	sq_getinteger(v,2,&i);
	__swap_dword((unsigned int *)&i);
	sq_pushinteger(v,i);
	return 1;
}

static int _g_blob_swapfloat(HSQUIRRELVM v)
{
	SQFloat f;
	sq_getfloat(v,2,&f);
	__swap_dword((unsigned int *)&f);
	sq_pushfloat(v,f);
	return 1;
}

#define _DECL_GLOBALBLOB_FUNC(name,nparams,typecheck) {_SC(#name),_g_blob_##name,nparams,typecheck}
static SQRegFunction bloblib_funcs[]={
	_DECL_GLOBALBLOB_FUNC(casti2f,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(castf2i,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap2,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swap4,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(swapfloat,2,_SC(".n")),
	_DECL_GLOBALBLOB_FUNC(blob,2,_SC(".n")),
	{0,0}
};

SQRESULT sqstd_getblob(HSQUIRRELVM v,int idx,SQUserPointer *ptr)
{
	SQBlob *blob;
	unsigned int typetag;
	if(SQ_FAILED(sq_getuserdata(v,idx,(SQUserPointer *)&blob,&typetag)) || typetag != SQSTD_BLOB_TYPE_TAG)
		return -1;
	*ptr = blob->GetBuf();
	return SQ_OK;
}

int sqstd_getblobsize(HSQUIRRELVM v,int idx)
{
	SQBlob *blob;
	unsigned int typetag;
	if(SQ_FAILED(sq_getuserdata(v,idx,(SQUserPointer *)&blob,&typetag)) || typetag != SQSTD_BLOB_TYPE_TAG)
		return -1;
	return blob->Len();
}

SQUserPointer sqstd_createblob(HSQUIRRELVM v, int size)
{
	SQUserPointer p = sq_newuserdata(v, sizeof(SQBlob));
	sq_setreleasehook(v,-1,_blob_releasehook);
	sq_settypetag(v,-1,SQSTD_BLOB_TYPE_TAG);
	new (p) SQBlob(size);
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_blob"),-1);
	sq_rawget(v,-2);
	sq_setdelegate(v,-3);
	sq_pop(v,1);
	return ((SQBlob *)p)->GetBuf();
}

SQRESULT sqstd_register_bloblib(HSQUIRRELVM v)
{
	//create delegate
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_blob"),-1);
	sq_newtable(v);
	int i = 0;
	while(_blob_delegate[i].name != 0) {
		SQRegFunction &f = _blob_delegate[i];
		sq_pushstring(v,f.name,-1);
		sq_newclosure(v,f.f,0);
		sq_setparamscheck(v,f.nparamscheck,f.typemask);
		sq_createslot(v,-3);
		i++;
	}
	sq_createslot(v,-3);
	sq_pop(v,1);

	i = 0;
	while(bloblib_funcs[i].name!=0)
	{
		SQRegFunction &f = bloblib_funcs[i];
		sq_pushstring(v,f.name,-1);
		sq_newclosure(v,f.f,0);
		sq_setparamscheck(v,f.nparamscheck,f.typemask);
		sq_createslot(v,-3);
		i++;
	}
	
	return SQ_OK;
}

