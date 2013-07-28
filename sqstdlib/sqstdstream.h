/*	see copyright notice in squirrel.h */
#ifndef _SQSTD_STREAM_H_
#define _SQSTD_STREAM_H_

#define SQSTD_STREAM_TYPE_TAG 0x80000000

struct SQStream {
	virtual SQInteger Read(void *buffer, SQInteger size) = 0;
	virtual SQInteger Write(void *buffer, SQInteger size) = 0;
	virtual int Flush() = 0;
	virtual long Tell() = 0;
	virtual SQInteger Len() = 0;
	virtual SQInteger Seek(long offset, int origin) = 0;
	virtual bool IsValid() = 0;
	virtual bool EOS() = 0;
};

int _stream_readstr(HSQUIRRELVM v);
int _stream_readblob(HSQUIRRELVM v);
int _stream_readline(HSQUIRRELVM v);
int _stream_readn(HSQUIRRELVM v);
int _stream_writestr(HSQUIRRELVM v);
int _stream_writeblob(HSQUIRRELVM v);
int _stream_writen(HSQUIRRELVM v);
int _stream_seek(HSQUIRRELVM v);
int _stream_tell(HSQUIRRELVM v);
int _stream_len(HSQUIRRELVM v);
int _stream_eos(HSQUIRRELVM v);
int _stream_flush(HSQUIRRELVM v);

#define _DECL_STREAM_FUNC(name,nparams,typecheck) {_SC(#name),_stream_##name,nparams,typecheck}

#endif /*_SQSTD_STREAM_H_*/
