/*	see copyright notice in squirrel.h */

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
//#include <conio.h>
#include <windows.h>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#include <conio.h>
#endif
#include <squirrel.h>
#include <sqbloblib.h>
#include <sqiolib.h>
#include <sqsystemlib.h>
#include <sqmathlib.h>
#include <time.h>
#include <math.h>

static int g_done=0;
static int g_lineinfo=0;
static int g_opt=0;
static int g_stacksize=1024;

int CompileScriptFromFile(HSQUIRRELVM,const SQChar *name,int printerror);
void PrintCallStack(HSQUIRRELVM);
void PrintVersionInfos();

int MemAllocHook( int allocType, void *userData, size_t size, int blockType, 
   long requestNumber, const unsigned char *filename, int lineNumber)
{
//	if(requestNumber==353)_asm int 3;
	return 1;
}

int printerror(HSQUIRRELVM v)
{
	const char *sErr=NULL;
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sErr)))	{
			fprintf(stderr,"\nAN ERROR HAS OCCURED [%s]\n",sErr);
		}
		else{
			fprintf(stderr,"\nAN ERROR HAS OCCURED [unknown]\n");
		}
		PrintCallStack(v);
	}
	return 0;
}

int compile_file(HSQUIRRELVM v)
{
	const char *sFileName;
	if(sq_gettop(v)>=1){
		if(SQ_SUCCEEDED(sq_getstring(v,2,&sFileName))){
			return CompileScriptFromFile(v,sFileName,0);
		}
	}
	return sq_throwerror(v,"(compile_file)wrong argument number");
}

int quit(HSQUIRRELVM v)
{
	g_done=1;
	return 0;
}

//<<FIXME>> this func is a mess
const char *getargs(HSQUIRRELVM v,int argc, char* argv[])
{
	int i;
	const char *ret=NULL;
	sq_pushroottable(v);
	sq_pushstring(v,"ARGS",-1);
	sq_newarray(v,0);
	
	if(argc>1)
	{
		int arg=1,exitloop=0;
		while(arg<argc && !exitloop)
		{

			if(argv[arg][0]=='-')
			{
				switch(argv[arg][1])
				{
				case 'd': //DEBUG(debug infos)
					g_lineinfo=1;
					g_opt=1;
					break;
				case 's': //STACKSIZE(debug infos)
					arg++;
					if(arg<argc){
						g_stacksize=atoi(argv[arg]);
						printf("stacksize=%d\n",g_stacksize);
						g_opt=1;
					}
					break;
				default:
					exitloop=1;
				}
			}else break;
			arg++;
		}
		// src file
		if(arg<argc)
			ret=argv[arg];
		arg++;
		for(i=arg;i<argc;i++)
		{
			sq_pushstring(v,argv[i],-1);
			sq_arrayappend(v,-2);
		}
	
	}
	sq_createslot(v,-3);
	sq_pop(v,1);
	return ret;
}


void PrintVersionInfos()
{
	fprintf(stdout,"%s %s\n",SQUIRREL_VERSION,SQUIRREL_COPYRIGHT);
}

void PrintUsage()
{
	fprintf(stderr,"usage: sq <options> <scriptpath [args]>.\n"
		"Available options are:\n"
		"   -d              generate debug infos\n"
		"   -s stacksize    initial stack size\n");
}

void PrintCallStack(HSQUIRRELVM v)
{
	SQStackInfos si;
	SQInteger i;
	SQFloat f;
	const SQChar *s;
	int level=1; //1 is to skip this function that is level 0
	const SQChar *name=NULL; 
	int seq=0;
	fprintf(stderr,"\nCALLSTACK\n");
	while(sq_stackinfos(v,level,&si))
	{
		const SQChar *fn="unknown";
		const SQChar *src="unknown";
		if(si.funcname)fn=si.funcname;
		if(si.source)src=si.source;
		fprintf(stderr,"*FUNCTION [%s] %s line [%d]\n",fn,src,si.line);
		level++;
	}
	level=0;
	fprintf(stderr,"\nLOCALS\n");
	
	for(level=0;level<10;level++){
		seq=0;
		while(name=sq_getlocal(v,level,seq))
		{
			seq++;
			switch(sq_gettype(v,-1))
			{
			case OT_NULL:
				fprintf(stderr,"[%s] NULL\n",name);
				break;
			case OT_INTEGER:
				sq_getinteger(v,-1,&i);
				fprintf(stderr,"[%s] %d\n",name,i);
				break;
			case OT_FLOAT:
				sq_getfloat(v,-1,&f);
				fprintf(stderr,"[%s] %.14g\n",name,f);
				break;
			case OT_USERPOINTER:
				fprintf(stderr,"[%s] USERPOINTER\n",name);
				break;
			case OT_STRING:
				sq_getstring(v,-1,&s);
				fprintf(stderr,"[%s] \"%s\"\n",name,s);
				break;
			case OT_TABLE:
				fprintf(stderr,"[%s] TABLE\n",name);
				break;
			case OT_ARRAY:
				fprintf(stderr,"[%s] ARRAY\n",name);
				break;
			case OT_CLOSURE:
				fprintf(stderr,"[%s] CLOSURE\n",name);
				break;
			case OT_NATIVECLOSURE:
				fprintf(stderr,"[%s] NATIVECLOSURE\n",name);
				break;
			case OT_USERDATA:
				fprintf(stderr,"[%s] USERDATA\n",name);
				break;
			}
			sq_pop(v,1);
		}
	}
}

int file_read(SQUserPointer file,SQUserPointer buf,int size)
{
	int ret;
	if( ( ret=fread(buf,1,size,(FILE *)file )!=0) )return ret;
	return -1;
}

int CompileScript(HSQUIRRELVM v,SQREADFUNC read,SQUserPointer p,const SQChar *sourcename,int bprinterror)
{
	int ret=-1;
	if(SQ_SUCCEEDED(sq_compile(v,read,p,sourcename,bprinterror,g_lineinfo))){
		ret=1;
	}
	return ret;
}

int CompileScriptFromFile(HSQUIRRELVM v,const char *filename,int bprinterror)
{
	FILE *file=fopen(filename,"rb");
	if(file){
		if(CompileScript(v,file_read,file,filename,bprinterror)>0){
			fclose(file);
			return 1;
		}
		fclose(file);
		return 0;
	}
	return sq_throwerror(v,"(dofile)cannot open the file");
}


int file_write(SQUserPointer file,SQUserPointer p,int size)
{
	return fwrite(p,1,size,(FILE *)file);
}

void compiler_error(HSQUIRRELVM v,const SQChar *sErr,const SQChar *sSource,int line,int column)
{
	fprintf(stderr,"ERROR %s line=(%d) column=(%d) [%s]",sErr,line,column,sSource);
}

typedef struct tagBufState{
	char *buf;
	int ptr;
	int size;
}BufState;

int read_buf(SQUserPointer file,SQUserPointer p,int size)
{
	BufState *buf=(BufState*)file;
	if(buf->size<(buf->ptr+size))
		return -1;
	memcpy(p,&buf->buf[buf->ptr],size);
	buf->ptr+=size;
	return size;
}

void Interactive(HSQUIRRELVM v)
{
	
#define MAXINPUT 1024
	char buffer[MAXINPUT];
	int blocks =0;
	int string=0;
	BufState bs;
	bs.buf=buffer;
	PrintVersionInfos();
	g_done=0;
	
    while (!g_done) 
	{
		int i = 0;
		bs.ptr=0;
		printf("sq>");
		for(;;) {
			int c;
			if(g_done)return;
			c = getchar();
			if (c == '\n') {
				if (i>0 && buffer[i-1] == '\\')
				{
					buffer[i-1] = '\n';
				}
				else if(blocks==0)break;
				buffer[i++] = '\n';
			}
			else if (c=='}') {blocks--; buffer[i++] = (char)c;}
			else if(c=='{' && !string){
					blocks++;
					buffer[i++] = (char)c;
			}
			else if(c=='"' || c=='\''){
					string=!string;
					buffer[i++] = (char)c;
			}
			else if (i >= MAXINPUT-1) {
				fprintf(stderr, "sq : input line too long\n");
				break;
			}
			else{
				buffer[i++] = (char)c;
			}
		}
		buffer[i] = '\0';
		bs.size=strlen(buffer);
		if(bs.size>0){
			int oldtop=sq_gettop(v);
			if(CompileScript(v,read_buf,&bs,"interactive console",1)>0){
				sq_pushroottable(v);
				sq_call(v,1,0);
			}
			printf("\n");
			sq_settop(v,oldtop);
		}
	}
}


void *x_malloc(unsigned int size){
	return malloc(size);
}

void *x_realloc(void *p,unsigned int oldsize,unsigned int size){
	return realloc(p,size);
}

void x_free(void *p,unsigned int size){
	free(p);
}

int main(int argc, char* argv[])
{
	HSQUIRRELVM v,v2;
	const char *filename=NULL;
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtSetAllocHook(MemAllocHook);
#endif
	
	///sq_open(x_malloc,x_realloc,x_free);
	v=sq_newvm(NULL,g_stacksize);
	
	sq_pushroottable(v);
	sq_blob_register(v,x_malloc,x_free);
	sq_iolib_register(v);
	sq_systemlib_register(v);
	sq_mathlib_register(v);

	
	//sets error handlers
	sq_setcompilererrorhandler(v,compiler_error);
	sq_newclosure(v,printerror,0);
	sq_seterrorhandler(v);

	sq_pushroottable(v);
	sq_pushstring(v,"compile_file",-1);
	sq_newclosure(v,compile_file,0);
	sq_createslot(v,-3);
	sq_pop(v,1);

	//gets arguments
	filename=getargs(v,argc,argv);

	if(filename && CompileScriptFromFile(v,filename,1)>0){
		/* SAVE & LOAD THE BYTECODE BEFORE EXECUTE IT(is just a sample)
		FILE *bytecodefile=fopen("./bytecode.cnut","wb+");
		sq_writeclosure(v,file_write,bytecodefile);
		fclose(bytecodefile);

		sq_pop(v,1);

		bytecodefile=fopen("./bytecode.cnut","rb");
		sq_readclosure(v,file_read,bytecodefile);
		fclose(bytecodefile);*/

		sq_pushroottable(v);
		sq_call(v,1,0);

	}
	else if(!filename && !g_opt){
		sq_pushroottable(v);
		sq_pushstring(v,"quit",-1);
		sq_newclosure(v,quit,0);
		sq_createslot(v,-3);
		sq_pop(v,1);
		Interactive(v);
	}
	else {
		PrintVersionInfos();
		PrintUsage();
	}
	sq_releasevm(v);
	
	//sq_close();
	
#if defined(_MSC_VER) && defined(_DEBUG)
	_getch();
	_CrtMemDumpAllObjectsSince( NULL );
#endif
	
	return 0;
}

