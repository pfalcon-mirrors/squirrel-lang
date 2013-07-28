
#include <stdio.h>
#include <string.h>
#include <new>
#include <squirrel.h>
#include "sqiolib.h"
#include "sqbloblib.h"

struct _File{
	_File()
	{
		m_pFile=NULL;
		m_bClose=true;
	}
	_File(FILE *f)
	{
		m_pFile=f;
		m_bClose=false;
	}
	~_File()
	{
		Close();
	}
	bool Eof()
	{
		if(!m_pFile)return false;
		return feof(m_pFile)!=0;
	}
	bool Open(const SQChar *name,const SQChar *mode)
	{
		m_pFile=fopen(name,mode);
		return m_pFile?true:false;
	}
	int Read(void *buf,int size)
	{
		if(!m_pFile)return false;
		return fread(buf,1,size,m_pFile);
	}
	int ReadLine(void *buf,int maxsize)
	{
		if(!m_pFile)return -1;
		memset(buf,0,maxsize);
		if(!fgets((char *)buf,maxsize,m_pFile))return -1;
		int n=strlen((char *)buf);
		if(((char *)buf)[n-1]=='\n')
		{
			((char *)buf)[n-1]='\0';
			return n-1;
		}
		return n;
	}
	int Write(void *buf,int size)
	{
		if(!m_pFile)return false;
		return fwrite(buf,size,1,m_pFile);
	}
	int Write(SQFloat f)
	{
		if(!m_pFile)return false;
		return fprintf(m_pFile,"%.14g",f);
	}
	int Write(SQInteger n)
	{
		if(!m_pFile)return false;
		return fprintf(m_pFile,"%d",n);
	}
	bool Seek(int mode,SQInteger n)
	{
		if(!m_pFile)return false;
		return !fseek(m_pFile,int(n),mode);
	}
	int Tell()
	{
		if(!m_pFile)return false;
		return ftell(m_pFile);
	}
	int Size()
	{
		int prevpos=Tell();
		Seek(SEEK_END,0);
		int size=Tell();
		Seek(SEEK_SET,prevpos);
		return size;
	}
	bool Close()
	{
		if(m_bClose && m_pFile)
		{
			fclose(m_pFile);
			return true;
		}
		return false;
	}
private:
	FILE *m_pFile;
	bool m_bClose;
};

static int file_release(SQUserPointer p)
{
	_File *f=(_File *)p;
	if(f)f->~_File();
	return 1;
}

static int io_fopen(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=4)
	{
//		SQObjectPtr &file=stack_get(v,2);
//		SQObjectPtr &mode=stack_get(v,3);
//		SQObjectPtr &delegate=stack_get(v,4);
		if(sq_gettype(v,3)!=OT_STRING ||
			sq_gettype(v,2)!=OT_STRING ||
			sq_gettype(v,4)!=OT_TABLE)
		{
			return sq_throwerror(v,"invalid argument type");
		}

		SQUserPointer ud=sq_newuserdata(v,sizeof(_File));
		_File *f=new (ud) _File;
		const SQChar *name,*mode;
		if(SQ_FAILED(sq_getstring(v,2,&name)) || SQ_FAILED(sq_getstring(v,3,&mode)) || !f->Open(name,mode))
		{
			f->~_File();
			sq_pop(v,1); //pop the userdata
			return sq_throwerror(v,"cannot open the file");
		}
		sq_setreleasehook(v,-1,file_release);
		sq_push(v,4);
		sq_setdelegate(v,-2);
		return 1;
	}
	return sq_throwerror(v,"invalid number of args");
}

static int io_file_close(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=1)
	{
		_File *f;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&f)))return sq_throwerror(v,"wrong ud");
		f->Close();
		return 0;
	}
	return sq_throwerror(v,"invalid number of args");
}

static int io_file_eof(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=1)
	{
		_File *f;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&f)))return sq_throwerror(v,"wrong ud");

		if(f->Eof()) sq_pushinteger(v,1);
		else sq_pushnull(v);

		return 1;
	}
	return sq_throwerror(v,"invalid number of args");
}


static int io_file_read(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		//SQObjectPtr &file=stack_get(v,1);
		//SQObjectPtr &arg=stack_get(v,2);
		_File *p;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&p)))return sq_throwerror(v,"wrong ud");
		switch(sq_gettype(v,2))
		{
		case OT_INTEGER:
			{
				SQInteger param;sq_getinteger(v,2,&param);
				switch(param){
				case 'l': {//line
					int nlen;
					if((nlen=p->ReadLine(sq_getscratchpad(v,1024),1024))>=0){
						sq_pushstring(v,sq_getscratchpad(v,-1),nlen);
						return 1;
					}else
						  return 0;}
					break;
				case 'c': //byte
					if(p->Read(sq_getscratchpad(v,1),1)){
						sq_pushinteger(v,(SQInteger)(sq_getscratchpad(v,-1)[0]));
						return 1;
					}
					break;
				case 'i':{ //integer
					SQInteger i;
					if(p->Read(&i,sizeof(SQInteger))){
						sq_pushinteger(v,i);
						return 1;
					}
						 }
					break;
				case 'f': { //float
					SQFloat f;
					if(p->Read(&f,sizeof(SQFloat))){
						sq_pushfloat(v,f);
						return 1;
					}
						 }
					break;
				case 's': { //short
					short s;
					if(p->Read(&s,sizeof(short))){
						sq_pushinteger(v,s);
						return 1;
					}
						 }
					break;
				case 'b': { //byte
					unsigned char b;
					if(p->Read(&b,sizeof(unsigned char))){
						sq_pushinteger(v,b);
						return 1;
					}
						 }
					break;
				case 'str':{
					if(sq_gettop(v)<3)return sq_throwerror(v,"missing param");
					SQInteger size,realsize;
					if(SQ_FAILED(sq_getinteger(v,3,&size)))return sq_throwerror(v,"wrong param");
					if(realsize=p->Read(sq_getscratchpad(v,size),size)){
						sq_pushstring(v,(sq_getscratchpad(v,-1)),realsize);
						return 1;
					}
						   }
					break;
				case 'a': //all file
					break;
				}
			}
			break;
		default:
			return sq_throwerror(v,"wrong argument");
			break;
		}

	}
	return sq_throwerror(v,"invalid number of args");
}

int io_file_readblob(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		_File *p;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer *)&p)))return sq_throwerror(v,"wrong ud");
		int size;sq_getinteger(v,2,&size);
		SQUserPointer blob=sq_blob_newblob(v,size,NULL,NULL);
		if(!p->Read(blob,size))return sq_throwerror(v,"error reading the file");
		return 1;
	}
	return sq_throwerror(v,"invalid number of args");
}

int io_file_writeblob(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		_File *p;
		if(SQ_FAILED(sq_blob_getblob(v,1,(SQUserPointer *)&p)))return sq_throwerror(v,"wrong ud");
		SQInteger blobsize=sq_blob_getblobsize(v,2);
		SQUserPointer blob;
		if(SQ_FAILED(sq_blob_getblob(v,2,&blob)))return sq_throwerror(v,"wrong arg");
		if(!p->Write(blob,blobsize))return sq_throwerror(v,"error writing the file");
		return 1;
	}
	return sq_throwerror(v,"invalid number of args");

}

int io_file_fillblob(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		_File *p;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer *)&p)))return sq_throwerror(v,"wrong ud");
		SQInteger blobsize=sq_blob_getblobsize(v,2);
		SQUserPointer blob;
		if(SQ_FAILED(sq_blob_getblob(v,2,&blob)))return sq_throwerror(v,"wrong arg");
		if(!p->Read(blob,blobsize))return sq_throwerror(v,"error reading the file");
		return 1;
	}
	return sq_throwerror(v,"invalid number of args");

}

static int io_file_write(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		SQInteger i;SQFloat f;
		const SQChar* s;
		_File *p;
		if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer *)&p)))return sq_throwerror(v,"wrong ud");
		
		switch(sq_gettype(v,2))
		{
		case OT_INTEGER:
			sq_getinteger(v,2,&i);
			p->Write(i);
			break;
		case OT_FLOAT:
			sq_getfloat(v,2,&f);
			p->Write(f);
			break;
		case OT_STRING:
			sq_getstring(v,2,&s);
			p->Write((void *)s,sq_getsize(v,2));
			
			break;
		default:
			return sq_throwerror(v,"wrong argument");
			break;
		}
		return 0;
		
	}
	return sq_throwerror(v,"invalid number of args");
}

int findstring (const SQChar *name, const SQChar *const list[]) {
  int i;
  for (i=0; list[i]; i++)
    if (strcmp(list[i], name) == 0)
      return i;
  return -1;  /* name not found */
}

static int io_file_seek(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2)
	{
		int op=-1,iorigin,realorigin=SEEK_SET;
		SQInteger offset=0;
		if(sq_gettop(v)>2){
			sq_getinteger(v,3,&iorigin);
			switch(iorigin){
				case 'set':realorigin=SEEK_SET;break;
				case 'cur':realorigin=SEEK_CUR;break;
				case 'end':realorigin=SEEK_END;break;
			}
		}
		if(sq_gettype(v,1)!=OT_USERDATA
			|| SQ_FAILED(sq_getinteger(v,2,&offset)))return sq_throwerror(v,"invalid agument");
		
		_File *p;
		sq_getuserdata(v,1,(SQUserPointer *)&p);
		if(p->Seek(realorigin,offset)){
			sq_pushinteger(v,p->Tell());
			return 1;
		}
		else{
			return sq_throwerror(v,"file op failure");
		}
		
	}
	return sq_throwerror(v,"invalid number of args");
}

static int io_file_typeof(HSQUIRRELVM v)
{
	sq_pushstring(v,"file",-1);
	return 1;
}

static int io_file_size(HSQUIRRELVM v)
{
	_File *p;
	if(SQ_FAILED(sq_getuserdata(v,1,(SQUserPointer*)&p)))return sq_throwerror(v,"invalid param");
	sq_pushinteger(v,p->Size());
	return 1;
}

int io_remove(HSQUIRRELVM v)
{
	const SQChar *s;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&s))){
		if(remove(s)==-1)return sq_throwerror(v,"remove() failed");
		
	}
	return sq_throwerror(v,"wrong param");
}

int io_rename(HSQUIRRELVM v)
{
	const SQChar *oldn,*newn;
	if(SQ_SUCCEEDED(sq_getstring(v,2,&oldn))
		&& SQ_SUCCEEDED(sq_getstring(v,3,&newn))){
		if(rename(oldn,newn)==-1)return sq_throwerror(v,"remove() failed");
		
	}
	return sq_throwerror(v,"wrong param");
}


static SQRegFunction io_funcs[]={
	{"remove",io_remove},
	{"rename",io_rename},
	{0,0}
};

static SQRegFunction io_file_funcs[]={
	{"close",io_file_close},
	{"eof",io_file_eof},
	{"read",io_file_read},
	{"readblob",io_file_readblob},
	{"writeblob",io_file_writeblob},
	{"fillblob",io_file_fillblob},
	{"write",io_file_write},
	{"seek",io_file_seek},
	{"size",io_file_size},
// reflex
	{"_typeof",io_file_typeof},
	{0,0}
};

int sq_iolib_register(HSQUIRRELVM v)
{
	int i=0;
	sq_pushstring(v,"fopen",-1);
	sq_newtable(v);
	////funcions
	while(io_file_funcs[i].name!=0)
	{
		sq_pushstring(v,io_file_funcs[i].name,-1);
		sq_newclosure(v,io_file_funcs[i].f,0);
		sq_createslot(v,-3);
		i++;
	}
	////
	sq_newclosure(v,io_fopen,1);
	sq_createslot(v,-3);

	//AHHHHH COPY & PASTE <<FIXME>>
	SQUserPointer ud=sq_newuserdata(v,sizeof(_File));
	_File *f=new (ud) _File(stdout);

	sq_push(v,-2);
	sq_setdelegate(v,-2);
	sq_setreleasehook(v,-1,file_release);

	sq_pushroottable(v);
	sq_pushstring(v,"stdout",-1);
	sq_push(v,-3);
	sq_createslot(v,-3);

	sq_pop(v,2);

	ud=sq_newuserdata(v,sizeof(_File));
	f=new (ud) _File(stdin);

	sq_push(v,-2);
	sq_setdelegate(v,-2);
	sq_setreleasehook(v,-1,file_release);

	sq_pushroottable(v);
	sq_pushstring(v,"stdin",-1);
	sq_push(v,-3);
	sq_createslot(v,-3);

	sq_pop(v,2);
	
	//
	
	i=0;
	while(io_funcs[i].name!=0)
	{
		sq_pushstring(v,io_funcs[i].name,-1);
		sq_newclosure(v,io_funcs[i].f,0);
		sq_createslot(v,-3);
		i++;
	}
	
	return SQ_OK;
}

