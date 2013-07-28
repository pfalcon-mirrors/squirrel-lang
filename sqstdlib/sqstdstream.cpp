#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <squirrel.h>
#include <sqstdio.h>
#include <sqstdblob.h>
#include "sqstdstream.h"
#include "sqstdblobimpl.h"

#define SETUP_STREAM(v) \
	SQStream *self = NULL; \
	sq_getuserdata(v,1,(SQUserPointer *)&self,NULL); \
	if(!self->IsValid())  \
		return sq_throwerror(v,_SC("the file was closed"));

int _stream_readstr(HSQUIRRELVM v)
{
    SETUP_STREAM(v);
	SQInteger type = _SC('a'), size = 0;
	sq_getinteger(v, 2, &size);
	if(size <= 0) return sq_throwerror(v,_SC("invalid size"));
	if(sq_gettop(v) > 2)
		sq_getinteger(v, 3, &type);
	SQChar *dest = NULL;
	switch(type) {
	case _SC('a'): {
		char *temp;
		if(self->Read(sq_getscratchpad(v, size+1), size) != size)
			return sq_throwerror(v, _SC("io failure"));
#ifdef _UNICODE
		temp = (char*) sq_getscratchpad(v, size + (size * sizeof(SQChar)));
		dest = (SQChar*) &temp[size];
		size = (SQInteger)mbstowcs(dest, (const char*)temp, size);
#else
		temp = (char *) sq_getscratchpad(v, -1);
		dest = temp;
#endif
				   }
		break;
	case _SC('u'): {
		wchar_t *temp;
		if(self->Read(sq_getscratchpad(v, (size + 1) * sizeof(wchar_t)),size * sizeof(wchar_t)) != (size * sizeof(wchar_t)))
			return sq_throwerror(v, _SC("io failure"));
		
#ifdef _UNICODE
		temp = (wchar_t*) sq_getscratchpad(v, -1);
		dest = (SQChar*) temp;
#else
		temp = (wchar_t*) sq_getscratchpad(v,(size * 3) + (size * sizeof(wchar_t)));
		dest = (char*) &temp[size];
		size = (SQInteger)wcstombs(dest, (const wchar_t*)temp, size);
#endif
				   }
		break;
	default:
		return sq_throwerror(v, _SC("invalid coding"));
	}

	sq_pushstring(v, dest, size);
	return 1;
}

int _stream_readblob(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	SQUserPointer data,blobp;
	SQInteger size,res;
	sq_getinteger(v,2,&size);
	if(size > self->Len()) {
		size = self->Len();
	}
	data = sq_getscratchpad(v,size);
	res = self->Read(data,size);
	if(res <= 0)
		return sq_throwerror(v,_SC("no data left to read"));
	blobp = sqstd_createblob(v,res);
	memcpy(blobp,data,res);
	return 1;
}


int _stream_readn(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	SQInteger format;
	sq_getinteger(v, 2, &format);
	switch(format) {
	case 'i': {
		int i;
		self->Read(&i, sizeof(i));
		sq_pushinteger(v, i);
			  }
		break;
	case 's': {
		short s;
		self->Read(&s, sizeof(short));
		sq_pushinteger(v, s);
			  }
		break;
	case 'w': {
		unsigned short w;
		self->Read(&w, sizeof(unsigned short));
		sq_pushinteger(v, w);
			  }
		break;
	case 'c': {
		char c;
		self->Read(&c, sizeof(char));
		sq_pushinteger(v, c);
			  }
		break;
	case 'b': {
		unsigned char c;
		self->Read(&c, sizeof(unsigned char));
		sq_pushinteger(v, c);
			  }
		break;
	case 'f': {
		float f;
		self->Read(&f, sizeof(float));
		sq_pushfloat(v, f);
			  }
		break;
	case 'd': {
		double d;
		self->Read(&d, sizeof(double));
		sq_pushfloat(v, (SQFloat)d);
			  }
		break;
	default:
		return sq_throwerror(v, _SC("invalid format"));
	}
	return 1;
}

int _stream_writestr(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	const SQChar *str,*res;
	SQInteger trgformat = 'a',len = 0;
	sq_getstring(v,2,&str);
	len = sq_getsize(v,2);
	if(sq_gettop(v)>2)
		sq_getinteger(v,3,&trgformat);
	switch(trgformat)
	{
	case 'a':
#ifdef _UNICODE
		res = sq_getscratchpad(v,len*3);
		len = (SQInteger) wcstombs((char *)res, (const wchar_t*)str, len);
#else
		res = str;
#endif
		self->Write((void *)res,len);
		break;
	case 'u':
#ifdef _UNICODE
		res = str;
#else
		res = sq_getscratchpad(v,len*sizeof(wchar_t));
		len = (SQInteger) mbstowcs((wchar_t*)res, str, len);
#endif
		self->Write((void *)res,len*sizeof(wchar_t));
		break;
	default:
		return sq_throwerror(v,_SC("wrong encoding"));
	}
	
	return 0;
}

int _stream_writeblob(HSQUIRRELVM v)
{
	SQUserPointer data;
	int size;
	SETUP_STREAM(v);
	if(SQ_FAILED(sqstd_getblob(v,2,&data)))
		return sq_throwerror(v,_SC("invalid parameter"));
	size = sqstd_getblobsize(v,2);
	if(self->Write(data,size) != size)
		return sq_throwerror(v,_SC("write error"));
	sq_pushinteger(v,size);
	return 1;
}

int _stream_writen(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	SQInteger format, ti;
	SQFloat tf;
	sq_getinteger(v, 3, &format);
	switch(format) {
	case 'i': {
		int i;
		sq_getinteger(v, 2, &ti);
		i = ti;
		self->Write(&i, sizeof(int));
			  }
		break;
	case 's': {
		short s;
		sq_getinteger(v, 2, &ti);
		s = ti;
		self->Write(&s, sizeof(short));
			  }
		break;
	case 'w': {
		unsigned short w;
		sq_getinteger(v, 2, &ti);
		w = ti;
		self->Write(&w, sizeof(unsigned short));
			  }
		break;
	case 'c': {
		char c;
		sq_getinteger(v, 2, &ti);
		c = ti;
		self->Write(&c, sizeof(char));
				  }
		break;
	case 'b': {
		unsigned char b;
		sq_getinteger(v, 2, &ti);
		b = ti;
		self->Write(&b, sizeof(unsigned char));
			  }
		break;
	case 'f': {
		float f;
		sq_getfloat(v, 2, &tf);
		f = tf;
		self->Write(&f, sizeof(float));
			  }
		break;
	case 'd': {
		double d;
		sq_getfloat(v, 2, &tf);
		d = tf;
		self->Write(&d, sizeof(double));
			  }
		break;
	default:
		return sq_throwerror(v, _SC("invalid format"));
	}
	return 0;
}

int _stream_seek(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	SQInteger offset, origin = SQ_SEEK_SET;
	sq_getinteger(v, 2, &offset);
	if(sq_gettop(v) > 2) {
		SQInteger t;
		sq_getinteger(v, 3, &t);
		switch(t) {
			case 'b': origin = SQ_SEEK_SET; break;
			case 'c': origin = SQ_SEEK_CUR; break;
			case 'e': origin = SQ_SEEK_END; break;
			default: return sq_throwerror(v,_SC("invalid origin"));
		}
	}
	sq_pushinteger(v, self->Seek(offset, origin));
	return 1;
}

int _stream_tell(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	sq_pushinteger(v, self->Tell());
	return 1;
}

int _stream_len(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	sq_pushinteger(v, self->Len());
	return 1;
}

int _stream_eos(HSQUIRRELVM v)
{
	SETUP_STREAM(v);
	if(self->EOS())
		sq_pushinteger(v, 1);
	else
		sq_pushnull(v);
	return 1;
}
