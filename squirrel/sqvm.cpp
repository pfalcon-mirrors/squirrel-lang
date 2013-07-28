/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "sqopcodes.h"
#include "sqfuncproto.h"
#include "sqvm.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqarray.h"

#define TOP() (_stack[_top-1])

#define ARITH_OP(op,_a1_,_a2_) { \
		const SQObjectPtr &o1=_a1_,&o2=_a2_; \
		if(sq_isnumeric(o1) && sq_isnumeric(o2)) { \
			if((type(o1)==OT_INTEGER) && (type(o2)==OT_INTEGER)) { \
				TARGET=_integer(o1) op _integer(o2); \
			}else{ \
				TARGET=tofloat(o1) op tofloat(o2); \
			}	\
		} else { \
		if(!ArithMetaMethod(#op[0],o1,o2,TARGET)) \
			sqraiseerror("arith op on wrong type"); \
		} \
	}

#define BW_OP(op) { \
		const SQObjectPtr &o1=STK(arg2),&o2=STK(arg1); \
		if((type(o1)==OT_INTEGER) && (type(o2)==OT_INTEGER)) { \
			TARGET=(SQInteger)(_integer(o1) op _integer(o2)); \
		} else sqraiseerror("applying bitwise op on non integer values"); \
	}


SQObjectPtr &stack_get(HSQUIRRELVM v,int idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

SQVM::~SQVM()
{
	_table(_roottable)->Clear();
	int size=_stack.size();
	for(int i=0;i<size;i++)
		_stack[i]=_null_;
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	RemoveFromChain(&GS->_vms_chain,this);
#endif
}

bool SQVM::ArithMetaMethod(int op,const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest)
{
	SQMetaMethod mm;
	switch(op){
		case '+': mm=MT_ADD; break;
		case '-': mm=MT_SUB; break;
		case '/': mm=MT_DIV; break;
		case '*': mm=MT_MUL; break;
	}
	switch(type(o1)){
		case OT_TABLE:
			if(_table(o1)->_delegate){
				Push(o1);Push(o2);
				return CallMetaMethod(_table(o1)->_delegate,mm,2,dest);
			}
			break;
		case OT_USERDATA:
			if(_userdata(o1)->_delegate){
				Push(o1);Push(o2);
				return CallMetaMethod(_userdata(o1)->_delegate,mm,2,dest);
			}
			break;
	}
	return false;
}

void SQVM::Modulo(const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest)
{
	if(sq_isnumeric(o1) && sq_isnumeric(o2)){
		if((type(o1)==OT_INTEGER ) && ( type(o2)==OT_INTEGER )){
			dest=SQInteger( _integer(o1)%_integer(o2) );
		}
		else{
			dest=SQFloat(fmod((double)tofloat(o1),(double)tofloat(o2))); 
		}
	}
	else{ 
		switch(type(o1)){
		case OT_TABLE:
			if(_table(o1)->_delegate){
				Push(o1);Push(o2);
				if(CallMetaMethod(_table(o1)->_delegate,MT_MODULO,2,dest))return;
			}
			break;
		case OT_USERDATA:
			if(_userdata(o1)->_delegate){
				Push(o1);Push(o2);
				if(CallMetaMethod(_userdata(o1)->_delegate,MT_MODULO,2,dest))return;
			}
			break;
		}
		sqraiseerror("arith op on wrong type");
	}
}

int SQVM::ObjCmp(const SQObjectPtr &o1,const SQObjectPtr &o2)
{
	if(type(o1)==type(o2)){
		if(_userpointer(o1)==_userpointer(o2))return 0;
		SQObjectPtr res;
		switch(type(o1)){
		case OT_STRING:
			return strcmp(_stringval(o1),_stringval(o2));
		case OT_INTEGER:
			return _integer(o1)-_integer(o2);
		case OT_FLOAT:
			return (_float(o1)<_float(o2))?-1:1;
		case OT_TABLE:
			Push(o1);Push(o2);
			if(_table(o1)->_delegate)CallMetaMethod(_table(o1)->_delegate,MT_CMP,2,res);
			break;
		case OT_USERDATA:
			Push(o1);Push(o2);
			if(_userdata(o1)->_delegate)CallMetaMethod(_userdata(o1)->_delegate,MT_CMP,2,res);
			break;
		}
		if(type(res)!=OT_INTEGER)sqraiseerror("comparing uncompatible types");
		return _integer(res);
	}
	else{
		if(sq_isnumeric(o1) && sq_isnumeric(o2)){
			if((type(o1)==OT_INTEGER) && (type(o2)==OT_FLOAT)){ 
				if( _integer(o1)==_float(o2) ) return 0;
				else if( _integer(o1)<_float(o2) ) return -1;
				return 1;
			}
			else{
				if( _float(o1)==_integer(o2) ) return 0;
				else if( _float(o1)<_integer(o2) ) return -1;
				return 1;
			}
		}
		else if(type(o1)==OT_NULL)return -1;
		else if(type(o2)==OT_NULL)return 1;
		else sqraiseerror("comparing uncompatible types"); 
		
	}
}

static inline void StringCat(const SQObjectPtr &str,const SQObjectPtr &obj,SQObjectPtr &dest)
{
	switch(type(obj))
	{
	case OT_STRING:
		switch(type(str)){
		case OT_STRING:	{
			int l=_string(str)->_len,ol=_string(obj)->_len;
			SQChar *s=_sp(_string(str)->_len+_string(obj)->_len+1);
			memcpy(s,_stringval(str),l);memcpy(s+l,_stringval(obj),ol);s[l+ol]='\0';
			break;
		}
		case OT_FLOAT:
			sprintf(_sp(NUMBER_MAX_CHAR+_string(obj)->_len+1),"%.14g%s",_float(str),_stringval(obj));
			break;
		case OT_INTEGER:
			sprintf(_sp(NUMBER_MAX_CHAR+_string(obj)->_len+1),"%d%s",_integer(str),_stringval(obj));
			break;
		default:
			sqraiseerror("_tostring reflex not implemented");
			break;
		}
		dest=SQString::Create(_spval);
		break;
	case OT_FLOAT:
		sprintf(_sp(NUMBER_MAX_CHAR+_string(str)->_len+1),"%s%.14g",_stringval(str),_float(obj));
		dest=SQString::Create(_spval);
		break;
	case OT_INTEGER:
		sprintf(_sp(NUMBER_MAX_CHAR+_string(str)->_len+1),"%s%d",_stringval(str),_integer(obj));
		dest=SQString::Create(_spval);
		break;
	default:
		assert(0);
		break;
	}
}

SQException::SQException(const SQChar *desc)
{
	_description=SQString::Create(desc);
}

SQException::SQException(const SQException &b)
{
	_description=b._description;
}

SQException::SQException(const SQObjectPtr &b)
{
	_description=b;
}

const SQChar *GetTypeName(SQObjectType type)
{
	switch(type)
	{
	case OT_NULL:return "null";
	case OT_INTEGER:return "integer";
	case OT_FLOAT:return "float";
	case OT_STRING:return "string";
	case OT_TABLE:return "table";
	case OT_ARRAY:return "array";
	case OT_GENERATOR:return "generator";
	case OT_CLOSURE:
	case OT_NATIVECLOSURE:
		return "function";
	case OT_USERDATA:
	case OT_USERPOINTER:
		return "userdata";
	case OT_FUNCPROTO:
		return "function";
	default:
		sqraiseerror("critical vm error");
	}
}

const SQChar *GetTypeName(const SQObjectPtr &obj1)
{
	return GetTypeName(type(obj1));	
}

void SQVM::TypeOf(const SQObjectPtr &obj1,SQObjectPtr &dest)
{
	switch(type(obj1))
	{
	case OT_TABLE:
		if(_table(obj1)->_delegate){
			Push(obj1);
			if(CallMetaMethod(_table(obj1)->_delegate,MT_TYPEOF,1,dest))
				return;
		}
		break;
	case OT_USERDATA:
		if(_userdata(obj1)->_delegate){
			Push(obj1);
			if(CallMetaMethod(_userdata(obj1)->_delegate,MT_TYPEOF,1,dest))
				return;
		}
	}
	dest=SQString::Create(GetTypeName(obj1));
}

SQVM::SQVM()
{
	_foreignptr=NULL;
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	AddToChain(&GS->_vms_chain,this);
#endif
}

bool SQVM::Init(int stacksize)
{
	_compilererrorhandler=NULL;
	_stack.resize(stacksize);
	_callsstack.reserve(16);
	_stackbase=0;
	_top=0;
	SQTable *rt=SQTable::Create(0);
	_roottable=rt;
	_refs_table=SQTable::Create(0);
	sq_base_register(this);
	return true;
}

void SQVM::ThrowError(const SQChar *err)
{
	_lasterror=SQString::Create(err);
}

extern SQInstructionDesc g_InstrDesc[];

void SQVM::StartCall(SQObjectPtr &closure,int target,int nargs,int stackbase,bool tailcall)
{
	SQFunctionProto *func=_funcproto(_closure(closure)->_function);
	const int outerssize=func->_outervalues.size();
	const int paramssize=func->_parameters.size()-outerssize;
	const unsigned int newtop=stackbase+func->_stacksize;
	const int oldtop=_top;
	if(!tailcall){
		PUSH_CALLINFO(this,CallInfo());
		ci->_prevstkbase=stackbase-_stackbase;
		ci->_target=target;
		ci->_prevtop=_top-_stackbase;
	}
	else{
		ci->_prevtop-=oldtop-newtop;
	}

	ci->_closure=closure;
	ci->_iv=&func->_instructions;
	ci->_literals=&func->_literals;
	//grow stack if needed
	if((newtop+1)>_stack.size()){
		_stack.resize(newtop+(newtop>>1));
	}
	//adjust stack(if there are too many or too few params
	if(paramssize!=nargs)
		if(paramssize<nargs)
			Pop(paramssize-nargs);
		else
			while(paramssize>nargs)
			{
				Push(_null_);
				nargs++;
			}
		
	if((outerssize)>0){
		int base=stackbase+paramssize;
		for(int i=0;i<outerssize;i++)
			_stack[base+i]=_closure(closure)->_outervalues[i];
	}
	_top=newtop;
	_stackbase=stackbase;
	ci->_ip=ci->_iv->_vals;
	ci->_root=false;
}

void IdxError(SQObject &o)
{
	char sTemp[150];
	char *s;
	switch(type(o)){
	case OT_STRING:
		s=_stringval(o);
		break;
	case OT_INTEGER:
		sprintf(_sp(NUMBER_MAX_CHAR+1),"%d",_integer(o));
		s=_spval;
		break;
	case OT_FLOAT:
		sprintf(_sp(NUMBER_MAX_CHAR+1),"%.14g",_float(o));
		s=_spval;
		break;
	default:
		s="???";
		break;
	}
	sprintf(sTemp,"the index '%s' does not exists",s);
	sqraiseerror(sTemp);
}

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

bool SQVM::Return(int _arg0,int _arg1,SQObjectPtr &retval)
{
	bool broot=ci->_root;
	int last_top=_top;
	int target=ci->_target;
	int oldstackbase=_stackbase;
	_stackbase-=ci->_prevstkbase;
	_top=_stackbase+ci->_prevtop;
	POP_CALLINFO(this);
	if(broot){
		if(_arg0!=-1) retval=_stack[oldstackbase+_arg1];
		else retval=_null_;
	}
	else{
		if(_arg0!=-1)
			STK(target)=_stack[oldstackbase+_arg1];
		else
			STK(target)=_null_;
	}

	while(last_top>=_top)_stack[last_top--]=_null_;
	assert(oldstackbase>=_stackbase);
	return broot;
}

#define arg0 (_i_._arg0)
#define sarg0 (*((char *)&(_i_._arg0)))
#define arg1 (_i_._arg1)
#define arg2 (_i_._arg2)
#define sarg2 (*((char *)&(_i_._arg2)))
#define larg2 (*((unsigned short *)&_i_._arg2))
#define arg3 (_i_._arg3)
#define sarg3 (*((char *)&(_i_._arg3)))

SQObjectPtr SQVM::Execute(SQObjectPtr &closure,int target,int nargs,int stackbase)
{
	int traps=0;
	//temp vars for OP_CALL
	int ct_target;
	bool ct_tailcall;
	StartCall(closure,_top-nargs,nargs,stackbase,false);
	ci->_root=true;

exception_restore:
	try{
		for(;;)
		{
			const SQInstruction &_i_=*ci->_ip++;
		//	dumpstack(_stackbase);
		//	printf("\n[%d] %s %d %d %d %d\n",ci->_ip-ci->_iv->_vals,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LOAD:
				TARGET=(*ci->_literals)[arg1];
				continue;
			case _OP_LOADNULL:
				TARGET=_null_;
				continue;
			case _OP_LOADROOTTABLE:
				TARGET=_roottable;
				continue;
			case _OP_MOVE:
				TARGET=STK(arg1)/*=STK(arg2)*/;
				continue;
			case _OP_NEWTABLE:
				TARGET=SQTable::Create(arg1);
				continue;
			case _OP_NEWARRAY:
				TARGET=SQArray::Create(0); _array(TARGET)->Reserve(arg1);
				continue;
			case _OP_APPENDARRAY:
				if(sarg2!=-1)
					_array(STK(arg0))->Append(STK(arg1));
				else
					_array(STK(arg0))->Append((*ci->_literals)[arg1]);
				continue;
			case _OP_NEWSLOT:
				NewSlot(STK(arg1),STK(arg2),STK(arg3));
				if(arg0!=arg3)
					TARGET=STK(arg3);
				continue;
			case _OP_DELETE:
				if(type(STK(arg1))==OT_TABLE){
					temp=_null_;
					if(_table(STK(arg1))->Get(STK(arg2),temp))
						_table(STK(arg1))->Remove(STK(arg2));
					TARGET=temp;
				}else sqraiseerror("the delete op works only on tables");
				continue;
			case _OP_SET:
				if(!Set(STK(arg1),STK(arg2),STK(arg3)))
					IdxError(STK(arg2));
				if(arg0!=arg3)
					TARGET=STK(arg3);
				continue;
			case _OP_GET:
				if(!Get(STK(arg1),STK(arg2),STK(arg0),false))
					IdxError(STK(arg2));
				continue;
			case _OP_GETK:
				if(!Get(STK(arg2),(*ci->_literals)[arg1],STK(arg0),false))
					IdxError((*ci->_literals)[arg1]);
				continue;
				break;
			case _OP_MINUSEQ:
				if(sarg3==-1){
					ARITH_OP( - ,STK(arg1),STK(arg2));
					if(arg1!=arg0)STK(arg1)=TARGET;
				}else{
					SQObjectPtr tmp,tself=STK(arg1),tkey=STK(arg2);
					if(!Get(tself,tkey,tmp,false))
						IdxError(tkey);
					ARITH_OP( - ,tmp,STK(arg3));
					if(!Set(tself,tkey,TARGET))
						IdxError(tkey);
				}
				continue;
			case _OP_PLUSEQ:
				if(sarg3==-1){
					ARITH_OP( + ,STK(arg1),STK(arg2));
					if(arg1!=arg0)STK(arg1)=TARGET;
				}else{
					SQObjectPtr tmp,tself=STK(arg1),tkey=STK(arg2);
					if(!Get(tself,tkey,tmp,false))
						IdxError(tkey);
					ARITH_OP( + ,tmp,STK(arg3));
					if(!Set(tself,tkey,TARGET))
						IdxError(tkey);
				}
				continue;
			case _OP_PREPCALL:{
					temp=STK(arg2);
					if(!Get(STK(arg2),STK(arg1),STK(arg0),false))
						IdxError(STK(arg1));
					STK(arg3)=temp;
				}
				continue;
			case _OP_PREPCALLK:{
					temp=STK(arg2);
					if(!Get(STK(arg2),(*ci->_literals)[arg1],STK(arg0),false))
						IdxError((*ci->_literals)[arg1]);
					STK(arg3)=temp;
				}
				continue;
			case _OP_JMP:
				ci->_ip+=(arg1);
				continue;
			case _OP_TAILCALL:
				temp=STK(arg1);
				if(type(temp)==OT_CLOSURE)
				{ 
					ct_tailcall=true;
					for(int i=0;i<arg3;i++)	STK(i)=STK(arg2+i);
					ct_target=ci->_target;
					goto common_call;
				}
			case _OP_CALL: {
					ct_tailcall=false;
					ct_target=arg0;
					temp=STK(arg1);
common_call:
					
					int last_top=_top;
					switch(type(temp)){
					case OT_CLOSURE:{
						StartCall(temp,ct_target,arg3,ct_tailcall?_stackbase:_stackbase+arg2,ct_tailcall);
						if(_closure(temp)->_bgenerator){
							SQGenerator *gen=SQGenerator::Create(_closure(temp));
							gen->Yield(this);
							Return(1,ct_target,temp);
							STK(ct_target)=gen;
							while(last_top>=_top)_stack[last_top--]=_null_;
							continue;
						}
						if(type(_debughook)!=OT_NULL && _rawval(_debughook)!=_rawval(ci->_closure))
							CallDebugHook('call');
						
						}
						break;
					case OT_NATIVECLOSURE:
						temp=CallNative(temp,arg3,ct_tailcall?_stackbase:_stackbase+arg2,ct_tailcall);
						STK(ct_target)=temp;
						break;
					case OT_TABLE:{
						Push(temp);
						for(int i=0;i<arg3;i++)	Push(STK(arg2+i));
						if(_table(temp)->_delegate && CallMetaMethod(_table(temp)->_delegate,MT_CALL,arg3+1,temp)){
							STK(ct_target)=temp;
							break;
						}
						sqraiseerror("calling a table")}
					case OT_USERDATA:
						if(_userdata(temp)->_delegate && CallMetaMethod(_userdata(temp)->_delegate,MT_CALL,arg3,temp)){
							STK(ct_target)=temp;
							break;
						}
						sqraiseerror("calling a userdata")
					default:
						sqraiseerror("calling a non closure value")
					}
				}
				  continue;
			case _OP_JNZ:
				if(type(STK(arg0))!=OT_NULL)ci->_ip+=(arg1);
				continue;
			case _OP_JZ:
				if(type(STK(arg0))==OT_NULL)ci->_ip+=(arg1);
				continue;

#define COND_LITERAL (arg3!=0?(*ci->_literals)[arg1]:STK(arg1))
			case _OP_ADD:{
				SQObjectPtr &o=COND_LITERAL;
				if(type(STK(arg2))!=OT_STRING && type(o)!=OT_STRING)
				{ARITH_OP( + ,STK(arg2),o);}
				else StringCat(STK(arg2),o,TARGET);
				}
				continue;
			case _OP_SUB:
				ARITH_OP( - ,STK(arg2),COND_LITERAL);continue;
			case _OP_MUL:
				ARITH_OP( * ,STK(arg2),COND_LITERAL);continue;
			case _OP_DIV:
				ARITH_OP( / ,STK(arg2),COND_LITERAL);continue;
			case _OP_MODULO:
				Modulo(STK(arg2),STK(arg1),TARGET);continue;
			case _OP_BWAND:
				BW_OP( & ); continue;
			case _OP_BWOR:
				BW_OP( | ); continue;
			case _OP_BWXOR:
				BW_OP( ^ ); continue;
			case _OP_SHIFTL:
				BW_OP( << ); continue;
			case _OP_SHIFTR:
				BW_OP( >> ); continue;
			case _OP_EQ:
				TARGET=(!ObjCmp(STK(arg2),STK(arg1))?_notnull_:_null_);continue;
			case _OP_NE:
				TARGET=(ObjCmp(STK(arg2),STK(arg1))?_notnull_:_null_);continue;
			case _OP_G:
				TARGET=((ObjCmp(STK(arg2),STK(arg1))>0)?_notnull_:_null_);continue;
			case _OP_GE:
				TARGET=((ObjCmp(STK(arg2),STK(arg1))>=0)?_notnull_:_null_);continue;
			case _OP_L:
				TARGET=((ObjCmp(STK(arg2),STK(arg1))<0)?_notnull_:_null_);continue;
			case _OP_LE:
				TARGET=((ObjCmp(STK(arg2),STK(arg1))<=0)?_notnull_:_null_);continue;
			case _OP_EXISTS:
				TARGET=Get(STK(arg1),STK(arg2),TARGET,true)?_notnull_:_null_;continue;
			case _OP_AND:
				if(type(STK(arg2))==OT_NULL){
					TARGET=STK(arg2);
					ci->_ip+=(arg1);
				}
				continue;
			case _OP_OR:
				if(type(STK(arg2))!=OT_NULL){
					TARGET=STK(arg2);
					ci->_ip+=(arg1);
				}
				continue;
			case _OP_NEG:{
					SQObjectPtr &o=STK(arg1);
					switch(type(o)){
					case OT_INTEGER:
						_integer(o)=-_integer(o);
						TARGET=o;
					continue;
					case OT_FLOAT:
						_float(o)=-_float(o);
						TARGET=o;
					continue;
					case OT_TABLE:
						if(_table(o)->_delegate
							&& CallMetaMethod(_table(o)->_delegate,MT_UNM,1,TARGET))
							continue;
						break;
					case OT_USERDATA:
						if(_userdata(o)->_delegate
							&& CallMetaMethod(_userdata(o)->_delegate,MT_UNM,1,TARGET))
							continue;
						break;
					}
					sqraiseerror("trying to negate a non number value");
				}
				continue;
				break;
			case _OP_NOT:
				TARGET=type(STK(arg1))==OT_NULL?_notnull_:_null_;
				continue;
			case _OP_BWNOT:
				if(type(STK(arg1))==OT_INTEGER){
					TARGET=SQInteger(~_integer(STK(arg1)));
					continue;
				}
				sqraiseerror("trying to apply a bitwise op on a non integer value");
			case _OP_CLOSURE:{
					int nouters;
					SQFunctionProto *func=_funcproto(_funcproto(_closure(ci->_closure)->_function)->_functions[arg1]);
					SQClosure *closure=SQClosure::Create(func);
					if(nouters=func->_outervalues.size()){
						closure->_outervalues.reserve(nouters);
						for(int i=0;i<nouters;i++){
							SQOuterVar &v=func->_outervalues[i];
							if(!v._blocal){//environment object
								closure->_outervalues.push_back(_null_);
								if(!Get(STK(0),v._src,closure->_outervalues.top(),false))
									IdxError(v._src);
							}
							else{//local
								closure->_outervalues.push_back(STK(_integer(v._src)));
							}
						}
					}
					TARGET=closure;
				}
				continue;
			case _OP_YIELD:{
				if(type(ci->_generator)==OT_GENERATOR){
					if(arg1!=-1)temp=STK(arg1);
					_generator(ci->_generator)->Yield(this);
					if(arg1!=-1)STK(arg1)=temp;
				}
				else sqraiseerror("only genenerator can be yielded");
				if(Return(sarg0,arg1,temp))
					return temp;
				}
				continue;
			case _OP_RESUME:
				if(type(STK(arg1))!=OT_GENERATOR)sqraiseerror("only generators can be resumed");
				_generator(STK(arg1))->Resume(this,arg0);
                continue;
			case _OP_RETURN:
				if(type((ci)->_generator)==OT_GENERATOR){
					_generator((ci)->_generator)->Kill();
				}
				if(Return(sarg0,arg1,temp))
					return temp;
				continue;
			case _OP_FOREACH:{
				int nrefidx;
				switch(type(STK(arg0))){
				case OT_TABLE:
					if((nrefidx=_table(STK(arg0))->Next(STK(arg2+2),STK(arg2),STK(arg2+1)))==-1)break;
					STK(arg2+2)=(SQInteger)nrefidx;continue;
				case OT_ARRAY:
					if((nrefidx=_array(STK(arg0))->Next(STK(arg2+2),STK(arg2),STK(arg2+1)))==-1)break;
					STK(arg2+2)=(SQInteger)nrefidx;continue;
				case OT_STRING:
					if((nrefidx=_string(STK(arg0))->Next(STK(arg2+2),STK(arg2),STK(arg2+1)))==-1)break;
					STK(arg2+2)=(SQInteger)nrefidx;continue;
				case OT_USERDATA:
					if(_userdata(STK(arg0))->_delegate){
						SQObjectPtr itr;
						Push(STK(arg0));
						Push(STK(arg2+2));
						if(CallMetaMethod(_userdata(STK(arg0))->_delegate,MT_NEXTI,2,itr)){
							STK(arg2+2)=STK(arg2)=itr;
							if(type(itr)==OT_NULL)break;
                            if(!Get(STK(arg0),itr,STK(arg2+1),false))
								sqraiseerror("_nexti returned an invalid idx");
							continue;
						}
					}
				case OT_GENERATOR:
					if(_generator(STK(arg0))->_state==SQGenerator::eDead)break;
					if(_generator(STK(arg0))->_state==SQGenerator::eSuspended){
						SQInteger idx=0;
						if(type(STK(arg2+2))==OT_INTEGER){
							idx=_integer(STK(arg2+2))+1;
						}
						STK(arg2)=idx;
						STK(arg2+2)=idx;
						_generator(STK(arg0))->Resume(this,arg2+1);
						continue;
					}
					default:
					sqraiseerror("iterating a non table/array/string value");
					break;
				}
				ci->_ip+=(arg1);
				continue;
							 }
			case _OP_DELEGATE:
				if(type(STK(arg1))!=OT_TABLE)sqraiseerror("delegating a non table");
				switch(type(STK(arg2))){
				case OT_TABLE:
					_table(STK(arg1))->SetDelegate(_table(STK(arg2)));
					break;
				case OT_NULL:
					_table(STK(arg1))->SetDelegate(NULL);
					break;
				default:
					sqraiseerror("can use only tables as delegate");
					break;
				}
				TARGET=STK(arg1);
				continue;
			case _OP_CLONE:
				switch(type(STK(arg1))){
				case OT_TABLE:
					TARGET=_table(STK(arg1))->Clone();
					if(_table(TARGET)->_delegate){
						Push(TARGET);
						Push(STK(arg1));
						CallMetaMethod(_table(TARGET)->_delegate,MT_CLONE,2,temp);
					}
					continue;
				case OT_ARRAY: 
					TARGET=_array(STK(arg1))->Clone();
					continue;
				default: sqraiseerror("cloning a non table");
				}
				continue;
			case _OP_TYPEOF:
				TypeOf(STK(arg1),TARGET);
				continue;
			case _OP_LINE:
				if(type(_debughook)!=OT_NULL && _rawval(_debughook)!=_rawval(ci->_closure))
					CallDebugHook('line');
				continue;
			case _OP_PUSHTRAP:
				ci->_etraps.push_back(SQExceptionTrap(_top,_stackbase,&ci->_iv->_vals[(ci->_ip-ci->_iv->_vals)+arg1],arg0));traps++;
				continue;
			case _OP_POPTRAP:
				ci->_etraps.pop_back();traps--;
				continue;
			case _OP_THROW:
				sqraiseerror(TARGET);
				continue;
			}
		}
	}
	catch(SQException &e)
	{
		int n=0;
		int last_top=_top;
		if(traps){
			do{
				if(ci->_etraps.size()){
					SQExceptionTrap &et=ci->_etraps.top();
					ci->_ip=et._ip;
					_top=et._stacksize;
					_stackbase=et._stackbase;
					_stack[_stackbase+et._extarget]=e._description;
					ci->_etraps.pop_back();traps--;
					while(last_top>=_top)_stack[last_top--]=_null_;
					goto exception_restore;
				}
				//if is a native closure
				if(type(ci->_closure)!=OT_CLOSURE && n) break;
				if(type(ci->_generator)==OT_GENERATOR)_generator(ci->_generator)->Kill();
				
				POP_CALLINFO(this);
				n++;
			}while(_callsstack.size());
		}
		//call the hook
		if(type(_errorhandler)!=OT_NULL){
			SQObjectPtr out;
			Push(_roottable);Push(e._description);
			Call(_errorhandler,2,_top-2,out);
			Pop(2);
		}
		//remove call stack until a C function is found or the cstack is empty
		do{
				
				if(type(ci->_generator)==OT_GENERATOR)_generator(ci->_generator)->Kill();
				_stackbase-=ci->_prevstkbase;
				_top=_stackbase+ci->_prevtop;
				POP_CALLINFO(this);
				if(type(ci->_closure)!=OT_CLOSURE) break;
		}while(_callsstack.size());
		while(last_top>=_top)_stack[last_top--]=_null_;
		//thow the exception to terminate the execution of the thread
		throw e;
	}
	return true;
}

void SQVM::CallDebugHook(int type)
{
	SQObjectPtr temp;
	SQFunctionProto *func=_funcproto(_closure(ci->_closure)->_function);
	Push(_roottable); Push(type); Push(func->GetLine(ci->_ip)); Push(func->_name);
	Call(_debughook,4,_top-4,temp);
	Pop(4);
}

SQObjectPtr SQVM::CallNative(SQObjectPtr &nclosure,int nargs,int stackbase,bool tailcall)
{
	int oldtop=_top;
	int oldstackbase=_stackbase;
	_top=stackbase+nargs;
	PUSH_CALLINFO(this,CallInfo());
	ci->_closure=nclosure;
	ci->_prevstkbase=stackbase-_stackbase;
	_stackbase=stackbase;
	//push outers
	int outers=_nativeclosure(nclosure)->_outervalues.size();
	for(int i=0;i<outers;i++){
		Push(_nativeclosure(nclosure)->_outervalues[i]);
	}
	ci->_prevtop=(oldtop-oldstackbase);
	int ret=(_nativeclosure(nclosure)->_function)(this);
	if(ret<0)sqraiseerror(_stringval(_lasterror));
	SQObjectPtr retval;
	if(ret!=0){ retval=TOP(); }
	_stackbase=oldstackbase;
	_top=oldtop;
	POP_CALLINFO(this);
	return retval;
}

bool SQVM::Get(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest,bool raw,bool root)
{
	switch(type(self)){
	case OT_TABLE:
		if(_table(self)->Get(key,dest))return true;
        //delegation
		if(_table(self)->_delegate){
			if(!Get(SQObjectPtr(_table(self)->_delegate),key,dest,raw,false)){
				if(raw)return false;
				//the stackbase param must not be 0 but something meaningful
				Push(self);Push(key);
				if(!CallMetaMethod(_table(self)->_delegate,MT_GET,2,dest)){
					return _table_ddel->Get(key,dest);
				}
			}
			return true;
		}
		return _table_ddel->Get(key,dest);
		break;
	case OT_ARRAY:
		if(sq_isnumeric(key)){
			return _array(self)->Get(tointeger(key),dest);
		}
		else return _array_ddel->Get(key,dest);
		return true;
	case OT_STRING:
		if(sq_isnumeric(key)){
			SQInteger n=tointeger(key);
			if(abs(n)<_string(self)->_len){
				if(n<0)n=_string(self)->_len-n;
				dest=SQInteger(_stringval(self)[n]);
				return true;
			}
			return false;
		}
		else return _string_ddel->Get(key,dest);
		break;
	case OT_USERDATA:{
		bool gret=false;
		if(_userdata(self)->_delegate
		&& !(gret=Get(SQObjectPtr(_userdata(self)->_delegate),key,dest,raw,false))){
			if(raw)return false;
			//the stackbase param must not be 0 but something meaningful
			Push(self);Push(key);
		 	if(!CallMetaMethod(_userdata(self)->_delegate,MT_GET,2,dest)){
				return false;
			}
			return true;
		}
		//meta table;
		return gret;
		}
		break;
	case OT_INTEGER:case OT_FLOAT:	return _number_ddel->Get(key,dest);
	case OT_GENERATOR:	return _generator_ddel->Get(key,dest);
	case OT_CLOSURE: case OT_NATIVECLOSURE:	return _closure_ddel->Get(key,dest);
	default:sqraiseerror("indexing a wrong type");
	}
}

bool SQVM::Set(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val)
{
	switch(type(self)){
	case OT_TABLE:
		if(!_table(self)->Set(key,val)){
			if(_table(self)->_delegate){
				if(Set(_table(self)->_delegate,key,val)){
					return true;
				}
				else{ //reflexivity (method __set)
					SQObjectPtr t;
					Push(self);Push(key);Push(val);
					return CallMetaMethod(_table(self)->_delegate,MT_SET,3,t);
				}
			}
			return false;
		}
		return true;
		break;
	case OT_ARRAY:
		if(!sq_isnumeric(key)) sqraiseerror("indexing an array with a non numerical index");
		return _array(self)->Set(tointeger(key),val);
		break;
	case OT_USERDATA:
		if(_userdata(self)->_delegate){
			SQObjectPtr t;
			Push(self);Push(key);Push(val);
			return CallMetaMethod(_userdata(self)->_delegate,MT_SET,3,t);
		}
		break;
	default:
		sqraiseerror("setting a wrong type");
		break;
	}
	return false;

}

void SQVM::NewSlot(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val)
{
	switch(type(self)){
	case OT_TABLE:
		if(type(key)==OT_NULL)sqraiseerror("null cannot be used as index");
		_table(self)->NewSlot(key,val);
		break;
	default:
		sqraiseerror("trying to add a field to a wrong type");
		break;
	}
}

bool SQVM::Call(SQObjectPtr &closure,int nparams,int stackbase,SQObjectPtr &outres)
{
	switch(type(closure)){
	case OT_CLOSURE:
		outres=Execute(closure,_top-nparams,nparams,stackbase);
		return true;
	case OT_NATIVECLOSURE:
		outres=CallNative(closure,nparams,stackbase,false);
		return true;
	default:
		return false;
	}
}

bool SQVM::CallMetaMethod(SQTable *mt,SQMetaMethod mm,int nparams,SQObjectPtr &outres)
{
	SQObjectPtr closure;
	if(mt->Get((*GS->_metamethods)[mm],closure)){
		if(Call(closure,nparams,_top-nparams,outres)){
			Pop(nparams);
			return true;
		}
	}
	Pop(nparams);
	return false;
}

#ifdef _DEBUG
void SQVM::dumpstack(int stackbase,bool dumpall)
{
	int size=dumpall?_stack.size():_top;
	int n=0;
	printf("\n>>>>stack dump<<<<\n");
	CallInfo &ci=_callsstack.back();
	printf("IP: %d\n",ci._ip);
	printf("prev stack base: %d\n",ci._prevstkbase);
	printf("prev top: %d\n",ci._prevtop);
	for(int i=0;i<size;i++){
		SQObjectPtr &obj=_stack[i];	
		if(stackbase==i)printf(">");else printf(" ");
		printf("[%d]:",n);
		switch(type(obj)){
		case OT_FLOAT:			printf("FLOAT %.3f",_float(obj));break;
		case OT_INTEGER:		printf("INTEGER %d",_integer(obj));break;
		case OT_STRING:			printf("STRING %s",_stringval(obj));break;
		case OT_NULL:			printf("NULL");	break;
		case OT_TABLE:			printf("TABLE %p[%p]",_table(obj),_table(obj)->_delegate);break;
		case OT_ARRAY:			printf("ARRAY %p",_array(obj));break;
		case OT_CLOSURE:		printf("CLOSURE");break;
		case OT_NATIVECLOSURE:	printf("NATIVECLOSURE");break;
		case OT_USERDATA:		printf("USERDATA %p[%p]",_userdataval(obj),_userdata(obj)->_delegate);break;
		case OT_GENERATOR:		printf("GENERATOR");break;
		case OT_USERPOINTER:	printf("USERPOINTER %p",_userpointer(obj));break;
		default:
			assert(0);
			break;
		};
		printf("\n");
		++n;
	}
}

#endif
