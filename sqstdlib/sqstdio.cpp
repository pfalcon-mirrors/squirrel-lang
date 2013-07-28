#include <new>
#include <stdio.h>
#include <squirrel.h>
#include <sqstdio.h>
#include "sqstdstream.h"

#define SQSTD_FILE_TYPE_TAG (SQSTD_STREAM_TYPE_TAG | 0x00000001)
//basic API
SQFILE sqstd_fopen(const SQChar *filename ,const SQChar *mode)
{
#ifndef _UNICODE
	return (SQFILE)fopen(filename,mode);
#else
	return (SQFILE)_wfopen(filename,mode);
#endif
}

SQInteger sqstd_fread(void* buffer, SQInteger size, SQInteger count, SQFILE file)
{
	return (SQInteger)fread(buffer,size,count,(FILE *)file);
}

SQInteger sqstd_fwrite(const SQUserPointer buffer, SQInteger size, SQInteger count, SQFILE file)
{
	return (SQInteger)fwrite(buffer,size,count,(FILE *)file);
}

int sqstd_fseek(SQFILE file, long offset, int origin)
{
	int realorigin;
	switch(origin) {
		case SQ_SEEK_CUR: realorigin = SEEK_CUR; break;
		case SQ_SEEK_END: realorigin = SEEK_END; break;
		case SQ_SEEK_SET: realorigin = SEEK_SET; break;
		default: return -1; //failed
	}
	return fseek((FILE *)file,offset,realorigin);
}

long sqstd_ftell(SQFILE file)
{
	return ftell((FILE *)file);
}

int sqstd_fflush(SQFILE file)
{
	return fflush((FILE *)file);
}

int sqstd_fclose(SQFILE file)
{
	return fclose((FILE *)file);
}

int sqstd_feof(SQFILE file)
{
	return feof((FILE *)file);
}

//File
struct SQFile : public SQStream {
	SQFile() { _handle = NULL; _owns = false;}
	SQFile(SQFILE file, bool owns) { _handle = file; _owns = owns;}
	~SQFile() { Close(); }
	bool Open(const SQChar *filename ,const SQChar *mode) {
		Close();
		if(_handle = sqstd_fopen(filename,mode)) {
			_owns = true;
			return true;
		}
		return false;
	}
	void Close() {
		if(_handle && _owns) { 
			sqstd_fclose(_handle);
			_handle = NULL;
			_owns = false;
		}
	}
	SQInteger Read(void *buffer,SQInteger size) {
		return sqstd_fread(buffer,1,size,_handle);
	}
	SQInteger Write(void *buffer,SQInteger size) {
		return sqstd_fwrite(buffer,1,size,_handle);
	}
	int Flush() {
		return sqstd_fflush(_handle);
	}
	long Tell() {
		return sqstd_ftell(_handle);
	}
	SQInteger Len() {
		int prevpos=Tell();
		Seek(0,SQ_SEEK_END);
		int size=Tell();
		Seek(prevpos,SQ_SEEK_SET);
		return size;
	}
	SQInteger Seek(long offset, int origin)	{
		return sqstd_fseek(_handle,offset,origin);
	}
	bool IsValid() { return _handle?true:false; }
	bool EOS() { return sqstd_feof(_handle)!=0?true:false;}
	SQFILE GetHandle() {return _handle;}
private:
	SQFILE _handle;
	bool _owns;
};

static int _file__typeof(HSQUIRRELVM v)
{
	sq_pushstring(v,_SC("file"),-1);
	return 1;
}


//bindings
#define _DECL_FILE_FUNC(name,nparams,typecheck) {_SC(#name),_file_##name,nparams,typecheck}
static SQRegFunction _file_delegate[] = {
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
	_DECL_FILE_FUNC(_typeof,1,_SC("u")),
	{0,0,0,0},
};

static int _file_releasehook(SQUserPointer p, int size)
{
	SQFile *self = (SQFile *)p;
	self->~SQFile();
	return 1;
}

SQRESULT sqstd_createfile(HSQUIRRELVM v, SQFILE file,int own)
{
	SQUserPointer p = sq_newuserdata(v, sizeof(SQFile));
	sq_setreleasehook(v,-1,_file_releasehook);
	sq_settypetag(v,-1,SQSTD_FILE_TYPE_TAG);
	new (p) SQFile(file, own?true:false);
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_io"),-1);
	if(SQ_FAILED(sq_rawget(v,-2)))
		return sq_throwerror(v,_SC("io lib not initialized"));
	sq_setdelegate(v,-3);
	sq_pop(v,1);
	return SQ_OK;
}

SQRESULT sqstd_getfile(HSQUIRRELVM v, int idx, SQFILE *file)
{
	SQFile *fileobj = NULL;
	unsigned int typetag;
	if(SQ_SUCCEEDED(sq_getuserdata(v,idx,(SQUserPointer*)&fileobj,&typetag))
		&& (typetag == SQSTD_FILE_TYPE_TAG)) {
			*file = fileobj->GetHandle();
			return SQ_OK;
		}
	return sq_throwerror(v,_SC("not a file"));
}


static int _io_fopen(HSQUIRRELVM v)
{
	const SQChar *filename,*mode;
	sq_getstring(v, 2, &filename);
	sq_getstring(v, 3, &mode);
	SQFILE newf = sqstd_fopen(filename, mode);
	if(!newf) return sq_throwerror(v, _SC("cannot open file"));
	if(SQ_FAILED(sqstd_createfile(v,newf,1)))
		return SQ_ERROR;
	return 1;
}

SQRESULT sqstd_register_iolib(HSQUIRRELVM v)
{
	//create delegate
	sq_pushregistrytable(v);
	sq_pushstring(v,_SC("std_io"),-1);
	sq_newtable(v);
	int i = 0;
	while(_file_delegate[i].name != 0) {
		SQRegFunction &f = _file_delegate[i];
		sq_pushstring(v,f.name,-1);
		sq_newclosure(v,f.f,0);
		sq_setparamscheck(v,f.nparamscheck,f.typemask);
		sq_createslot(v,-3);
		i++;
	}
	sq_createslot(v,-3);
	sq_pop(v,1); //pops registry

	sq_pushstring(v,_SC("fopen"),-1);
    sq_newclosure(v,_io_fopen,0);
	sq_setparamscheck(v,3,_SC(".ss"));
	sq_createslot(v,-3);
	
	return SQ_OK;
}
