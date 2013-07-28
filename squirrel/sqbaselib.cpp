/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

int sq_aux_checkargs(HSQUIRRELVM v,int nargs,int ncheck,...)
{
	if(sq_gettop(v)<nargs)return sq_throwerror(v,"invalid num of args");
	int count = 0, sum = 0;
	SQObjectType type;
   va_list marker;

   va_start( marker, ncheck );     /* Initialize variable arguments. */
   for(int i=0;i<ncheck;i++)
   {
      type = va_arg( marker, SQObjectType);
	  if(sq_gettype(v,i+1)!=type)return sq_throwerror(v,"invalid num of args");
   }
   va_end( marker );              /* Reset variable arguments.      */
   return 1;

}

bool str2num(const SQChar *s,SQObjectPtr &res)
{
	SQChar *end;
	if(strstr(s,".")){
		SQFloat r=SQFloat(strtod(s,&end));
		if(s==end) return false;
		while (isspace((unsigned char)(*end))) end++;
		if (*end != '\0') return false;
		res = r;
		return true;
	}
	else{
		const SQChar *t=s;
		while(*t!='\0')if(!isdigit(*t++))return false;
		res=SQInteger(atoi(s));
		return true;
	}
	
}

#ifdef GARBAGE_COLLECTOR
static int base_collect_garbage(HSQUIRRELVM v)
{
	sq_pushinteger(v,GS->CollectGarbage(v));
	return 1;
}
#endif

static int base_getroottable(HSQUIRRELVM v)
{
	v->Push(v->_roottable);
	return 1;
}

static int base_setroottable(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,2,0)))return -1;
	SQObjectPtr &o=stack_get(v,2);
	sq_setroottable(v);
	v->Push(o);
	return 1;
}

static int base_seterrorhandler(HSQUIRRELVM v)
{
	sq_seterrorhandler(v);
	return 0;
}

static int base_setdebughook(HSQUIRRELVM v)
{
	sq_setdebughook(v);
	return 0;
}

static int base_assert(HSQUIRRELVM v)
{
	if(sq_gettop(v)==2){
		if(sq_gettype(v,-1)!=OT_NULL){
			return 0;
		}
		else return sq_throwerror(v,"assertion failed");
	}
	return sq_throwerror(v,"wrong number of params");
}

static int get_slice_params(HSQUIRRELVM v,int &sidx,int &eidx,SQObjectPtr &o)
{
	int top;
	if((top=sq_gettop(v))>=2){
		sidx=0;
		eidx=0;
		o=stack_get(v,1);
		SQObjectPtr &start=stack_get(v,2);
		if(type(start)!=OT_NULL && sq_isnumeric(start)){
			sidx=tointeger(start);
			
		}
		if(top>2){
			SQObjectPtr &end=stack_get(v,3);
			if(sq_isnumeric(end)){
				eidx=tointeger(end);
			}
		}
		return 1;
	}return sq_throwerror(v,"wrong number of params");
}

static int base_print(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2){
		SQObjectPtr &o=stack_get(v,2);
		switch(type(o)){
		case OT_STRING:
			printf("%s",_stringval(o));
			break;
		case OT_INTEGER:
			printf("%d",_integer(o));
			break;
		case OT_FLOAT:
			printf("%.14g",_float(o));
			break;
		default:{
			SQObjectPtr tname;
			v->TypeOf(o,tname);
			printf("(%s)",_stringval(tname));
			}
			break;
		}
		return 0;
	}
	return sq_throwerror(v,"wrong number of params");
}

static int base_ascii2string(HSQUIRRELVM v)
{
	if(sq_gettop(v)>=2){
		SQObject &o=stack_get(v,2);
		if(type(o)&SQOBJECT_NUMERIC){
			unsigned char c=tointeger(o);
			v->Push(SQString::Create((const SQChar *)&c,1));
			return 1;
		}return sq_throwerror(v,"has to be a number");
	}
	return sq_throwerror(v,"wrong number of params");
}


static SQRegFunction base_funcs[]={
	//generic
	{"seterrorhandler",base_seterrorhandler},
	{"setdebughook",base_setdebughook},
	{"getroottable",base_getroottable},
	{"setroottable",base_setroottable},
	{"assert",base_assert},
	{"print",base_print},
	//string stuff
	{"ascii2string",base_ascii2string},
#ifdef GARBAGE_COLLECTOR
	{"collect_garbage",base_collect_garbage},
#endif
	{0,0}
};

void sq_base_register(HSQUIRRELVM v)
{
	int i=0;
	sq_pushroottable(v);
	while(base_funcs[i].name!=0){
		sq_pushstring(v,base_funcs[i].name,-1);
		sq_newclosure(v,base_funcs[i].f,0);
		sq_createslot(v,-3);
		i++;
	}
	sq_pop(v,1);
}

static int default_delegate_len(HSQUIRRELVM v)
{
	v->Push(SQInteger(sq_getsize(v,1)));
	return 1;
}

static int default_delegate_tofloat(HSQUIRRELVM v)
{
	if(sq_gettop(v)==1){
		SQObjectPtr &o=stack_get(v,1);
		switch(type(o)){
		case OT_STRING:{
			SQObjectPtr res;
			if(str2num(_stringval(o),res)){
				v->Push(SQObjectPtr(tofloat(res)));
				break;
			}}
		case OT_INTEGER:case OT_FLOAT:
			v->Push(SQObjectPtr(tofloat(o)));
			break;
		default:
			v->Push(_null_);
			break;
		}
		return 1;
	}
	return sq_throwerror(v,"wrong number of params");
}

static int default_delegate_tointeger(HSQUIRRELVM v)
{
	if(sq_gettop(v)==1){
		SQObjectPtr &o=stack_get(v,1);
		switch(type(o)){
		case OT_STRING:{
			SQObjectPtr res;
			if(str2num(_stringval(o),res)){
				v->Push(SQObjectPtr(tointeger(res)));
				break;
			}}
		case OT_INTEGER:case OT_FLOAT:
			v->Push(SQObjectPtr(tointeger(o)));
			break;
		default:
			v->Push(_null_);
			break;
		}
		return 1;
	}
	return sq_throwerror(v,"wrong number of params");
}

static int default_delegate_tostring(HSQUIRRELVM v)
{
	if(sq_gettop(v)==1){
		SQObjectPtr &o=stack_get(v,1);
		switch(type(o)){
		case OT_STRING:
			v->Push(o);
			break;
		case OT_INTEGER:
			sprintf(_sp(NUMBER_MAX_CHAR+1),"%d",_integer(o));
			v->Push(SQString::Create(_spval));
			break;
		case OT_FLOAT:
			sprintf(_sp(NUMBER_MAX_CHAR+1),"%.14g",_float(o));
			v->Push(SQString::Create(_spval));
			break;
		default:
			v->Push(_null_);
			break;
		}
		return 1;
	}
	return sq_throwerror(v,"wrong number of params");
}

/////////////////////////////////////////////////////////////////
//TABLE DEFAULT DELEGATE

static int table_getdelegate(HSQUIRRELVM v)
{
	return sq_getdelegate(v,-1);
}


static int table_rawset(HSQUIRRELVM v)
{
	return sq_rawset(v,-3);
}


static int table_rawget(HSQUIRRELVM v)
{
	return sq_rawget(v,-2);
}

SQRegFunction SQGlobalState::_table_default_delegate_funcz[]={
	{"len",default_delegate_len},
	{"rawget",table_rawget},
	{"rawset",table_rawset},
	{"getdelegate",table_getdelegate},
	{0,0}
};

//ARRAY DEFAULT DELEGATE///////////////////////////////////////

static int array_append(HSQUIRRELVM v)
{
	return sq_arrayappend(v,-2);
}

static int array_reverse(HSQUIRRELVM v)
{
	return sq_arrayreverse(v,-1);
}

static int array_pop(HSQUIRRELVM v)
{
	return sq_arraypop(v,1,1);
}

static int array_top(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,1,1,OT_ARRAY)))return -1;
	SQObject &o=stack_get(v,1);
	if(_array(o)->Size()>0){
		v->Push(_array(o)->Top());
		return 1;
	}
	else return sq_throwerror(v,"top() on a empty array");
}

static int array_insert(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,3,1,OT_ARRAY)))return -1;
	SQObject &o=stack_get(v,1);
	SQObject &idx=stack_get(v,2);
	SQObject &val=stack_get(v,3);
	_array(o)->Insert(idx,val);
	return 0;
}

static int array_remove(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,2,1,OT_ARRAY)))return -1;
	SQObject &o=stack_get(v,1);
	SQObject &idx=stack_get(v,2);
	if(!sq_isnumeric(idx))return sq_throwerror(v,"wrong type");
	SQObjectPtr val;
	if(_array(o)->Get(tointeger(idx),val)){
		_array(o)->Remove(idx);
		v->Push(val);
		return 1;
	}
	return sq_throwerror(v,"idx out of range");
}

static int array_resize(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,2,1,OT_ARRAY)))return -1;
	SQObject &o=stack_get(v,1);
	SQObject &nsize=stack_get(v,2);
	if(sq_isnumeric(nsize)){
		_array(o)->Resize(tointeger(nsize));
		return 0;
	}
	return sq_throwerror(v,"size must be a number");
}

//QSORT ala Sedgewick
void _qsort(HSQUIRRELVM v,SQArray &a, int l, int r)
{
	int i, j;
	SQObjectPtr pivot,t;
	if( l < r ){
		pivot = a._values[l];
		i = l; j = r+1;
		while(1){
			do ++i; while((i <= r) && (v->ObjCmp(a._values[i],pivot) <= 0));
			do --j; while( v->ObjCmp(a._values[j],pivot) > 0 );
			if( i >= j ) break;
			t = a._values[i]; a._values[i] = a._values[j]; a._values[j] = t;
		}
		t = a._values[l]; a._values[l] = a._values[j]; a._values[j] = t;
		_qsort( v, a, l, j-1);
		_qsort( v, a, j+1, r);
	}
}

static int array_sort(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,1,1,OT_ARRAY)))return -1;
	SQObject &o=stack_get(v,1);
	if(_array(o)->Size()>1){
		_qsort(v,*_array(o),0,_array(o)->Size()-1);
	}
	return 0;
}

static int array_slice(HSQUIRRELVM v)
{
	int sidx,eidx;
	SQObjectPtr o;
	if(SQ_FAILED(sq_aux_checkargs(v,1,1,OT_ARRAY)))return -1;
	if(get_slice_params(v,sidx,eidx,o)==-1)return -1;
	if(sidx<0)sidx=_array(o)->Size()+sidx;
	if(eidx<1)eidx=_array(o)->Size()+eidx;
	if(eidx<=sidx)return sq_throwerror(v,"wrog indexes");
	SQArray *arr=SQArray::Create(eidx-sidx);
	SQObjectPtr t;
	int count=0;
	for(int i=sidx;i<eidx;i++){
		_array(o)->Get(i,t);
		arr->Set(count++,t);
	}
	v->Push(arr);
	return 1;
	
}

SQRegFunction SQGlobalState::_array_default_delegate_funcz[]={
	{"len",default_delegate_len},
	{"append",array_append},
	{"push",array_append},
	{"pop",array_pop},
	{"top",array_top},
	{"insert",array_insert},
	{"remove",array_remove},
	{"resize",array_resize},
	{"reverse",array_reverse},
	{"sort",array_sort},
	{"slice",array_slice},
	{0,0}
};

//STRING DEFAULT DELEGATE//////////////////////////
static int string_slice(HSQUIRRELVM v)
{
	int sidx,eidx;
	SQObjectPtr o;
	if(SQ_FAILED(get_slice_params(v,sidx,eidx,o)))return -1;
	if(sidx<0)sidx=_string(o)->_len+sidx;
	if(eidx<1)eidx=_string(o)->_len+eidx;
	if(eidx<=sidx)return sq_throwerror(v,"wrog indexes");
	v->Push(SQString::Create(&_stringval(o)[sidx],eidx-sidx));
	return 1;
}

#define STRING_TOFUNCZ(func) static int string_##func(HSQUIRRELVM v) \
{ \
	if(SQ_FAILED(sq_aux_checkargs(v,1,1,OT_STRING)))return -1; \
	SQObject str=stack_get(v,1); \
	int len=_string(str)->_len; \
	const SQChar *sThis=_stringval(str); \
	SQChar *sNew=_sp(len); \
	for(int i=0;i<len;i++) sNew[i]=func(sThis[i]); \
	v->Push(SQString::Create(sNew,len)); \
	return 1; \
}


STRING_TOFUNCZ(tolower)
STRING_TOFUNCZ(toupper)

SQRegFunction SQGlobalState::_string_default_delegate_funcz[]={
	{"len",default_delegate_len},
	{"tointeger",default_delegate_tointeger},
	{"tofloat",default_delegate_tofloat},
	{"slice",string_slice},
	{"tolower",string_tolower},
	{"toupper",string_toupper},
	{0,0}
};

//INTEGER DEFAULT DELEGATE//////////////////////////
SQRegFunction SQGlobalState::_number_default_delegate_funcz[]={
	{"tointeger",default_delegate_tointeger},
	{"tofloat",default_delegate_tofloat},
	{"tostring",default_delegate_tostring},
	{0,0}
};

//CLOSURE DEFAULT DELEGATE//////////////////////////
static int closure_call(HSQUIRRELVM v)
{
	return sq_call(v,sq_gettop(v)-1,1);
}

static int closure_acall(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,2,2,OT_CLOSURE,OT_ARRAY)))return -1;
	SQArray *aparams=_array(stack_get(v,2));
	int nparams=aparams->Size();
	v->Push(stack_get(v,1));
	for(int i=0;i<nparams;i++)v->Push(aparams->_values[i]);
	return sq_call(v,nparams,1);
}

SQRegFunction SQGlobalState::_closure_default_delegate_funcz[]={
	{"call",closure_call},
	{"acall",closure_acall},
	{0,0}
};

//GENERATOR DEFAULT DELEGATE
static int generator_getstatus(HSQUIRRELVM v)
{
	if(SQ_FAILED(sq_aux_checkargs(v,1,1,OT_GENERATOR)))return -1;
	SQObject &o=stack_get(v,1);
	switch(_generator(o)->_state){
		case SQGenerator::eSuspended:v->Push(SQString::Create("suspended"));break;
		case SQGenerator::eRunning:v->Push(SQString::Create("running"));break;
		case SQGenerator::eDead:v->Push(SQString::Create("dead"));break;
	}
	return 1;
}

SQRegFunction SQGlobalState::_generator_default_delegate_funcz[]={
	{"getstatus",generator_getstatus},
	{0,0}
};
