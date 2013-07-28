/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
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

#define ARITH_OP(op,trg,_a1_,_a2_) { \
		const SQObjectPtr &o1=_a1_,&o2=_a2_; \
		if(sq_isnumeric(o1) && sq_isnumeric(o2)) { \
			if((type(o1)==OT_INTEGER) && (type(o2)==OT_INTEGER)) { \
				trg=_integer(o1) op _integer(o2); \
			}else{ \
				trg=tofloat(o1) op tofloat(o2); \
			}	\
		} else { \
			if(#op[0] == '+' &&	(type(o1) == OT_STRING || type(o2) == OT_STRING)) \
					StringCat(o1, o2, trg); \
			else if(!ArithMetaMethod(#op[0],o1,o2,trg)) \
				RT_Error(_SC("arith op %c on between '%s' and '%s'"),#op[0],GetTypeName(o1),GetTypeName(o2)); \
		} \
	}

#define BW_OP(op) { \
		const SQObjectPtr &o1=STK(arg2),&o2=STK(arg1); \
		if((type(o1)==OT_INTEGER) && (type(o2)==OT_INTEGER)) { \
			TARGET=(SQInteger)(_integer(o1) op _integer(o2)); \
		} else RT_Error(_SC("bitwise op between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2)); \
	}


SQObjectPtr &stack_get(HSQUIRRELVM v,int idx){return ((idx>=0)?(v->GetAt(idx+v->_stackbase-1)):(v->GetUp(idx)));}

SQVM::SQVM(SQSharedState *ss)
{
	_sharedstate=ss;
	_suspended=false;
	_suspended_target=-1;
	_suspended_root=false;
	_suspended_traps=-1;
	_foreignptr=NULL;
	_nnativecalls=0;
	_uiRef=0;
	_lasterror = _null_;
	_errorhandler = _null_;
	_debughook = _null_;
	INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);
}

void SQVM::Finalize()
{
	_roottable = _null_;
	_lasterror = _null_;
	_errorhandler = _null_;
	_debughook = _null_;
	temp_reg = _null_;
	int size=_stack.size();
	for(int i=0;i<size;i++)
		_stack[i]=_null_;
}

SQVM::~SQVM()
{
	Finalize();
	REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
}

bool SQVM::ArithMetaMethod(int op,const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest)
{
	SQMetaMethod mm;
	switch(op){
		case _SC('+'): mm=MT_ADD; break;
		case _SC('-'): mm=MT_SUB; break;
		case _SC('/'): mm=MT_DIV; break;
		case _SC('*'): mm=MT_MUL; break;
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
		RT_Error(_SC("arith op on between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2));
	}
}

int SQVM::ObjCmp(const SQObjectPtr &o1,const SQObjectPtr &o2)
{
	if(type(o1)==type(o2)){
		if(_userpointer(o1)==_userpointer(o2))return 0;
		SQObjectPtr res;
		switch(type(o1)){
		case OT_STRING:
			return scstrcmp(_stringval(o1),_stringval(o2));
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
		if(type(res)!=OT_INTEGER)CompareError(o1,o2);
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
		else CompareError(o1,o2);
		
	}
	return 0; //cannot happen
}

void SQVM::StringCat(const SQObjectPtr &str,const SQObjectPtr &obj,SQObjectPtr &dest)
{
	switch(type(obj))
	{
	case OT_STRING:
		switch(type(str)){
		case OT_STRING:	{
			int l=_string(str)->_len,ol=_string(obj)->_len;
			SQChar *s=_sp(rsl(l+ol+1));
			memcpy(s,_stringval(str),rsl(l));memcpy(s+l,_stringval(obj),rsl(ol));s[l+ol]=_SC('\0');
			break;
		}
		case OT_FLOAT:
			scsprintf(_sp(rsl(NUMBER_MAX_CHAR+_string(obj)->_len+1)),_SC("%g%s"),_float(str),_stringval(obj));
			break;
		case OT_INTEGER:
			scsprintf(_sp(rsl(NUMBER_MAX_CHAR+_string(obj)->_len+1)),_SC("%d%s"),_integer(str),_stringval(obj));
			break;
		default:
			RT_Error(_SC("string concatenation between '%s' and '%s'"),GetTypeName(str),GetTypeName(obj));
		}
		dest=SQString::Create(_ss(this),_spval);
		break;
	case OT_FLOAT:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+_string(str)->_len+1)),_SC("%s%g"),_stringval(str),_float(obj));
		dest=SQString::Create(_ss(this),_spval);
		break;
	case OT_INTEGER:
		scsprintf(_sp(rsl(NUMBER_MAX_CHAR+_string(str)->_len+1)),_SC("%s%d"),_stringval(str),_integer(obj));
		dest=SQString::Create(_ss(this),_spval);
		break;
	default:RT_Error(_SC("string concatenation between '%s' and '%s'"),GetTypeName(str),GetTypeName(obj));
	}
}

SQException::SQException(SQSharedState *ss,const SQChar *str)
{
	_description=SQString::Create(ss,str);
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
	switch(_RAW_TYPE(type))
	{
	case _RT_NULL:return _SC("null");
	case _RT_INTEGER:return _SC("integer");
	case _RT_FLOAT:return _SC("float");
	case _RT_STRING:return _SC("string");
	case _RT_TABLE:return _SC("table");
	case _RT_ARRAY:return _SC("array");
	case _RT_GENERATOR:return _SC("generator");
	case _RT_CLOSURE:
	case _RT_NATIVECLOSURE:
		return _SC("function");
	case _RT_USERDATA:
	case _RT_USERPOINTER:
		return _SC("userdata");
	case _RT_THREAD:
		return _SC("thread");
	case _RT_FUNCPROTO:
		return _SC("function");
	default:
		return NULL;
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
	dest=SQString::Create(_ss(this),GetTypeName(obj1));
}



bool SQVM::Init(SQVM *friendvm, int stacksize)
{
	
	_stack.resize(stacksize);
	_callsstack.reserve(4);
	_stackbase = 0;
	_top = 0;
	if(!friendvm) 
		_roottable = SQTable::Create(_ss(this), 0);
	else {
		_roottable = friendvm->_roottable;
		_errorhandler = friendvm->_errorhandler;
		_debughook = friendvm->_debughook;
	}
	
	sq_base_register(this);
	return true;
}

extern SQInstructionDesc g_InstrDesc[];

void SQVM::StartCall(SQClosure *closure,int target,int nargs,int stackbase,bool tailcall)
{
	SQFunctionProto *func = _funcproto(closure->_function);
	const int outerssize = func->_outervalues.size();
	const int paramssize = func->_parameters.size()-outerssize;
	const int newtop = stackbase + func->_stacksize;
	const int oldtop = _top;

	if (paramssize != nargs)
		RT_Error(_SC("wrong number of parameters"));

	if (!tailcall) {
		PUSH_CALLINFO(this, CallInfo());
		ci->_etraps = 0;
		ci->_prevstkbase = stackbase - _stackbase;
		ci->_target = target;
		ci->_prevtop = _top - _stackbase;
		ci->_ncalls = 1;
		ci->_root = false;
	}
	else {
		ci->_ncalls++;
	}

	ci->_closure._unVal.pClosure = closure;
	ci->_closure._type = OT_CLOSURE;
	ci->_iv = &func->_instructions;
	ci->_literals = &func->_literals;
	//grows the stack if needed
	if (((unsigned int)newtop + (func->_stacksize<<1)) > _stack.size()) {
		_stack.resize(_stack.size() + (func->_stacksize<<1));
	}
		
	if ((outerssize) > 0) {
		int base = stackbase + paramssize;
		for (int i = 0; i < outerssize; i++)
			_stack[base+i] = closure->_outervalues[i];
	}
	_top = newtop;
	_stackbase = stackbase;
	ci->_ip = ci->_iv->_vals;
	
}

#define STK(a) _stack._vals[_stackbase+(a)]
#define TARGET _stack._vals[_stackbase+arg0]

bool SQVM::Return(int _arg0, int _arg1, SQObjectPtr &retval)
{
	if (type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
		for(int i=0;i<ci->_ncalls;i++)
			CallDebugHook(_SC('r'));
						
	bool broot = ci->_root;
	int last_top = _top;
	int target = ci->_target;
	int oldstackbase = _stackbase;
	_stackbase -= ci->_prevstkbase;
	_top = _stackbase + ci->_prevtop;
	POP_CALLINFO(this);
	if (broot) {
		if (_arg0 != 0xFF) retval = _stack[oldstackbase+_arg1];
		else retval = _null_;
	}
	else {
		if (_arg0 != 0xFF)
			STK(target) = _stack[oldstackbase+_arg1];
		else
			STK(target) = _null_;
	}

	while (last_top >= _top) _stack[last_top--] = _null_;
	assert(oldstackbase >= _stackbase); 
	return broot;
}


#define LOCAL_INC(target, a, incr) \
{ \
	ARITH_OP( + , target, a, incr); \
	a = target; \
}

#define PLOCAL_INC(target, a, incr) \
{ \
 	SQObjectPtr trg; \
	ARITH_OP( + , trg, a, incr); \
	target = a; \
	a = trg; \
}

void SQVM::DerefInc(SQObjectPtr &target, SQObjectPtr &self, SQObjectPtr &key, SQObjectPtr &incr, bool postfix)
{
	SQObjectPtr tmp, tself = self, tkey = key;
	if (!Get(tself, tkey, tmp, false)) IdxError(tkey);
	ARITH_OP( + , target, tmp, incr);
	Set(tself, tkey, target);
	if (postfix) target = tmp;
}

#define arg0 (_i_._arg0)
#define arg1 (_i_._arg1)
#define sarg1 (*((short *)&_i_._arg1))
#define arg2 (_i_._arg2)
#define arg3 (_i_._arg3)


SQRESULT SQVM::Suspend()
{
	if (_suspended)
		return sq_throwerror(this, _SC("cannot suspend an already suspended vm"));
	if (_nnativecalls!=2)
		return sq_throwerror(this, _SC("cannot suspend through native calls/metamethods"));
	return SQ_SUSPEND_FLAG;
}

SQObjectPtr SQVM::Execute(SQObjectPtr &closure, int target, int nargs, int stackbase, ExecutionType et)
{
	if ((_nnativecalls + 1) > MAX_NATIVE_CALLS) RT_Error(_SC("Native stack overflow"));
	_nnativecalls++;
	AutoDec ad(&_nnativecalls);
	int traps = 0;
	//temp_reg vars for OP_CALL
	int ct_target;
	bool ct_tailcall;
	bool protectedcall = true;
	SQ_TRY {
		switch(et){
			case ET_CALL:
			case ET_UNPROTECTEDCALL:
				StartCall(_closure(closure), _top - nargs, nargs, stackbase, false); ci->_root = true;
				protectedcall = !(et == ET_UNPROTECTEDCALL);
				if (type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
					CallDebugHook(_SC('c'));
				break;
			case ET_RESUME_GENERATOR: _generator(closure)->Resume(this, target); ci->_root = true; traps += ci->_etraps; break;
			case ET_RESUME_VM:
				traps = _suspended_traps;
				ci->_root = _suspended_root;
				_suspended = false;
				break;
		}
	}
	SQ_CATCH(SQException,e) { 
//call the handler if there are no calls in the stack, if not relies on the previous node
		if(ci == NULL) CallErrorHandler(e);
		throw e;
	}

exception_restore:
	SQ_TRY {
		for(;;)
		{
			const SQInstruction &_i_ = *ci->_ip++;
			//dumpstack(_stackbase);
			//scprintf("\n[%d] %s %d %d %d %d\n",ci->_ip-ci->_iv->_vals,g_InstrDesc[_i_.op].name,arg0,arg1,arg2,arg3);
			switch(_i_.op)
			{
			case _OP_LOAD: TARGET = (*ci->_literals)[arg1]; continue;
			case _OP_LOADNULL: TARGET = _null_; continue;
			case _OP_LOADNULLS:{ for(int n=0;n<arg1;n++) STK(arg0+n) = _null_; }continue;
			case _OP_LOADROOTTABLE:	TARGET = _roottable; continue;
			case _OP_MOVE: TARGET = STK(arg1); continue;
			case _OP_DMOVE: STK(arg0) = STK(arg1); STK(arg2) = STK(arg3); continue;
			case _OP_NEWTABLE: TARGET = SQTable::Create(_ss(this), arg1); continue;
			case _OP_NEWARRAY: TARGET = SQArray::Create(_ss(this), 0); _array(TARGET)->Reserve(arg1); continue;
			case _OP_APPENDARRAY:
				if(arg2!=0xFF)
					_array(STK(arg0))->Append(STK(arg1));
				else
					_array(STK(arg0))->Append((*ci->_literals)[arg1]);
				continue;
			case _OP_NEWSLOT:
				NewSlot(STK(arg1), STK(arg2), STK(arg3));
				if(arg0 != arg3)
					TARGET = STK(arg3);
				continue;
			case _OP_DELETE: DeleteSlot(STK(arg1), STK(arg2), TARGET); continue;
			case _OP_SET:
				if (!Set(STK(arg1), STK(arg2), STK(arg3))) IdxError(STK(arg2));
				if (arg0 != arg3)
					TARGET = STK(arg3);
				continue;
			case _OP_GET:
				if (!Get(STK(arg1), STK(arg2), temp_reg, false)) IdxError(STK(arg2));
				TARGET = temp_reg;
				continue;
			case _OP_GETK:
				if (!Get(STK(arg2), (*ci->_literals)[arg1], temp_reg, false)) IdxError((*ci->_literals)[arg1]);
				TARGET = temp_reg;
				continue;
				break;
			case _OP_GETPARENT:
				if(type(STK(arg1)) == OT_TABLE) {
                  		TARGET = _table(STK(arg1))->_delegate?SQObjectPtr(_table(STK(arg1))->_delegate):_null_;
						continue;
				}
				RT_Error(_SC("the %s type doesn't have a parent slot"), GetTypeName(STK(arg1)));
				break;
			case _OP_MINUSEQ:
				if (arg3 == 0xFF) {
					ARITH_OP( - ,temp_reg, STK(arg1), STK(arg2)); TARGET = temp_reg;
					if(arg1 != arg0) STK(arg1) = TARGET;
				}
				else {
					SQObjectPtr tmp, tself = STK(arg1), tkey = STK(arg2);
					if(!Get(tself, tkey, tmp, false)) IdxError(tkey);
					ARITH_OP( - , temp_reg, tmp, STK(arg3));  TARGET = temp_reg;
					if(!Set(tself, tkey, TARGET)) IdxError(tkey);
				}
				continue;
			case _OP_PLUSEQ:
				if (arg3 == 0xFF) LOCAL_INC(TARGET, STK(arg1), STK(arg2))
				else DerefInc(TARGET, STK(arg1), STK(arg2), STK(arg3), false);
				continue;
			case _OP_INC:
				if (arg3 == 0xFF) LOCAL_INC(TARGET, STK(arg1), _one_)
				else DerefInc(TARGET, STK(arg1), STK(arg2), _one_, false);
				continue;
			case _OP_PINC:
				if (arg3 == 0xFF) PLOCAL_INC(TARGET, STK(arg1), _one_)
				else DerefInc(TARGET, STK(arg1), STK(arg2), _one_, true);
				continue;
			case _OP_DEC:
				if (arg3 == 0xFF) LOCAL_INC(TARGET, STK(arg1), _minusone_)
				else DerefInc(TARGET, STK(arg1), STK(arg2), _minusone_, false);
				continue;
			case _OP_PDEC:
				if (arg3 == 0xFF) PLOCAL_INC(TARGET, STK(arg1), _minusone_)
				else DerefInc(TARGET, STK(arg1), STK(arg2), _minusone_, true);
				continue;
			case _OP_PREPCALL:{
					SQObjectPtr tmp = STK(arg2);
					if (!Get(STK(arg2), STK(arg1), temp_reg, false))
						IdxError(STK(arg1));
					STK(arg3) = tmp;
					TARGET = temp_reg;
				}
				continue;
			case _OP_PREPCALLK:{
					SQObjectPtr tmp = STK(arg2);
					if (!Get(STK(arg2), (*ci->_literals)[arg1], temp_reg,false))
						IdxError((*ci->_literals)[arg1]);
					STK(arg3) = tmp;
					TARGET = temp_reg;
				}
				continue;
			case _OP_JMP: ci->_ip += (sarg1); continue;
			case _OP_TAILCALL:
				temp_reg = STK(arg1);
				if (type(temp_reg) == OT_CLOSURE){ 
					ct_tailcall = true;
					for (int i = 0; i < arg3; i++) STK(i) = STK(arg2 + i);
					ct_target = ci->_target;
					goto common_call;
				}
			case _OP_CALL: {
					ct_tailcall = false;
					ct_target = arg0;
					temp_reg = STK(arg1);
common_call:
					int last_top = _top;
					switch (type(temp_reg)) {
					case OT_CLOSURE:{
						StartCall(_closure(temp_reg), ct_target, arg3, ct_tailcall?_stackbase:_stackbase+arg2, ct_tailcall);
						if (_funcproto(_closure(temp_reg)->_function)->_bgenerator) {
							SQGenerator *gen = SQGenerator::Create(_ss(this), _closure(temp_reg));
							gen->Yield(this);
							Return(1, ct_target, temp_reg);
							STK(ct_target) = gen;
							while (last_top >= _top) _stack[last_top--] = _null_;
							continue;
						}
						if (type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
							CallDebugHook(_SC('c'));
						}
						break;
					case OT_NATIVECLOSURE:
						if (CallNative(_nativeclosure(temp_reg), arg3, _stackbase+arg2, ct_tailcall, temp_reg)){
							_suspended = true;
							_suspended_target = ct_target;
							_suspended_root = ci->_root;
							_suspended_traps = traps;
							return temp_reg;
						}
						STK(ct_target) = temp_reg;
						break;
					case OT_TABLE:{
						Push(temp_reg);
						for (int i = 0; i < arg3; i++) Push(STK(arg2 + i));
						if (_table(temp_reg)->_delegate && CallMetaMethod(_table(temp_reg)->_delegate, MT_CALL, arg3+1, temp_reg)){
							STK(ct_target) = temp_reg;
							break;
						}
						RT_Error(_SC("attempt to call '%s'"), GetTypeName(temp_reg));
								  }
					case OT_USERDATA:{
						Push(temp_reg);
						for (int i = 0; i < arg3; i++) Push(STK(arg2 + i));
						if (_userdata(temp_reg)->_delegate && CallMetaMethod(_userdata(temp_reg)->_delegate, MT_CALL, arg3+1, temp_reg)){
							STK(ct_target) = temp_reg;
							break;
						}
						RT_Error(_SC("attempt to call '%s'"), GetTypeName(temp_reg));
									 }
					default:
						RT_Error(_SC("attempt to call '%s'"), GetTypeName(temp_reg));
					}
				}
				  continue;
			case _OP_JNZ: if (type(STK(arg0)) != OT_NULL) ci->_ip+=(sarg1); continue;
			case _OP_JZ:  if (type(STK(arg0)) == OT_NULL) ci->_ip+=(sarg1); continue;

#define COND_LITERAL (arg3!=0?(*ci->_literals)[arg1]:STK(arg1))
			case _OP_ADD: ARITH_OP( + , temp_reg, STK(arg2), COND_LITERAL); TARGET = temp_reg; continue;
			case _OP_SUB: ARITH_OP( - , temp_reg, STK(arg2), COND_LITERAL); TARGET = temp_reg; continue;
			case _OP_MUL: ARITH_OP( * , temp_reg, STK(arg2), COND_LITERAL); TARGET = temp_reg; continue;
			case _OP_DIV: {
				const SQObjectPtr &a = STK(arg2), &b = COND_LITERAL;
				if(type(a) == OT_INTEGER &&	type(b) == OT_INTEGER && _integer(b) == 0)
					RT_Error(_SC("division by zero"));
				ARITH_OP( / , temp_reg, a, b); TARGET = temp_reg; continue;
						  }
			case _OP_MODULO: Modulo(STK(arg2), STK(arg1), temp_reg); TARGET = temp_reg; continue;
			case _OP_BWAND:	 BW_OP( & ); continue;
			case _OP_BWOR:	 BW_OP( | ); continue;
			case _OP_BWXOR:	 BW_OP( ^ ); continue;
			case _OP_SHIFTL: BW_OP( << ); continue;
			case _OP_SHIFTR: BW_OP( >> ); continue;
			case _OP_USHIFTR: { 
					const SQObjectPtr &o1=STK(arg2),&o2=STK(arg1); 
					if((type(o1)==OT_INTEGER) && (type(o2)==OT_INTEGER)) { 
						TARGET = (SQInteger)(*((unsigned int*)&o1._unVal.nInteger) >> _integer(o2)); 
					} else RT_Error(_SC("bitwise op between '%s' and '%s'"),GetTypeName(o1),GetTypeName(o2)); 
							  }
                continue;
			case _OP_EQ:{
				SQObjectPtr &o1 = STK(arg2);
				SQObjectPtr &o2 = COND_LITERAL;
				TARGET= type(o1) == type(o2)?
							((_userpointer(o1) == _userpointer(o2)?_notnull_:_null_)):
								(sq_isnumeric(o1) && sq_isnumeric(o2)?
									((!ObjCmp(STK(arg2), COND_LITERAL)?_notnull_:_null_))
							:_null_);
					}continue;
			case _OP_NE:{ 
				SQObjectPtr &o1 = STK(arg2); 
				SQObjectPtr &o2 = COND_LITERAL; 
				if(type(o1) != type(o2)) { 
					TARGET = sq_isnumeric(o1) && sq_isnumeric(o2)? 
							((tofloat(o1) != tofloat(o2))? _notnull_ : _null_) 
							:_notnull_; 
					} 
				else { 
					TARGET = _userpointer(o1) != _userpointer(o2)?_notnull_:_null_; 
				} 
					} continue; 
			case _OP_G:		 TARGET = ((ObjCmp(STK(arg2), COND_LITERAL) > 0)?_notnull_:_null_);	continue;
			case _OP_GE:	 TARGET = ((ObjCmp(STK(arg2), COND_LITERAL) >= 0)?_notnull_:_null_);	continue;
			case _OP_L:		 TARGET = ((ObjCmp(STK(arg2), COND_LITERAL) < 0)?_notnull_:_null_);	continue;
			case _OP_LE:	 TARGET = ((ObjCmp(STK(arg2), COND_LITERAL) <= 0)?_notnull_:_null_);	continue;
			case _OP_EXISTS: TARGET = Get(STK(arg1), STK(arg2), temp_reg, true)?_notnull_:_null_;continue;
			case _OP_AND:
				if(type(STK(arg2)) == OT_NULL) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_OR:
				if(type(STK(arg2)) != OT_NULL) {
					TARGET = STK(arg2);
					ci->_ip += (sarg1);
				}
				continue;
			case _OP_NEG:{
					SQObjectPtr &o = STK(arg1);
					switch(type(o)) {
					case OT_INTEGER:
						TARGET = -_integer(o);
					continue;
					case OT_FLOAT:
						TARGET = -_float(o);

						continue;
					case OT_TABLE:
						if(_table(o)->_delegate) {
							Push(o);
							if(CallMetaMethod(_table(o)->_delegate, MT_UNM, 1, temp_reg)) {
								TARGET = temp_reg;
							continue;
							}
						}
						break;
					case OT_USERDATA:
						if(_userdata(o)->_delegate) {
							Push(o);
							if(CallMetaMethod(_userdata(o)->_delegate, MT_UNM, 1, temp_reg)){
								TARGET = temp_reg;
							continue;
							}
						}
						break;
					}
					RT_Error(_SC("attempt to negate a %s"), GetTypeName(STK(arg1)));
				}
				continue;
				break;
			case _OP_NOT: TARGET = (type(STK(arg1)) == OT_NULL?_notnull_:_null_); continue;
			case _OP_BWNOT:
				if(type(STK(arg1)) == OT_INTEGER) {
					TARGET = SQInteger(~_integer(STK(arg1)));
					continue;
				}
				RT_Error(_SC("attempt to perform a bitwise op on a %s"), GetTypeName(STK(arg1)));
			case _OP_CLOSURE:{
					int nouters;
					SQFunctionProto *func = _funcproto(_funcproto(_closure(ci->_closure)->_function)->_functions[arg1]);
					SQClosure *closure = SQClosure::Create(_ss(this), func);
					if(nouters = func->_outervalues.size()) {
						closure->_outervalues.reserve(nouters);
						for(int i = 0; i<nouters; i++) {
							SQOuterVar &v = func->_outervalues[i];
							if(!v._blocal) {//environment object
								closure->_outervalues.push_back(_null_);
								if(!Get(STK(0), v._src, closure->_outervalues.top(), false))
									IdxError(v._src);
							}
							else {//local
								closure->_outervalues.push_back(STK(_integer(v._src)));
							}
						}
					}
					TARGET = closure;
				}
				continue;
			case _OP_YIELD:{
				if(type(ci->_generator) == OT_GENERATOR) {
					if(sarg1 != -1) temp_reg = STK(arg1);
					_generator(ci->_generator)->Yield(this);
					traps -= ci->_etraps;
					if(sarg1 != -1) STK(arg1) = temp_reg;
				}
				else RT_Error(_SC("trying to yield a '%s',only genenerator can be yielded"), GetTypeName(ci->_generator));
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					return temp_reg;
				}
					
				}
				continue;
			case _OP_RESUME:
				if(type(STK(arg1)) != OT_GENERATOR)RT_Error(_SC("trying to resume a '%s',only genenerator can be resumed"), GetTypeName(STK(arg1)));
				_generator(STK(arg1))->Resume(this, arg0);
				traps += ci->_etraps;
                continue;
			case _OP_RETURN:
				if(type((ci)->_generator) == OT_GENERATOR) {
					_generator((ci)->_generator)->Kill();
				}
				if(Return(arg0, arg1, temp_reg)){
					assert(traps==0);
					return temp_reg;
				}
				continue;
			case _OP_FOREACH: {
				int nrefidx;
				switch(type(STK(arg0))) {
				case OT_TABLE:
					if((nrefidx = _table(STK(arg0))->Next(STK(arg2+2), STK(arg2), STK(arg2+1))) == -1) break;
					STK(arg2+2) = (SQInteger)nrefidx; continue;
				case OT_ARRAY:
					if((nrefidx = _array(STK(arg0))->Next(STK(arg2+2), STK(arg2), STK(arg2+1))) == -1) break;
					STK(arg2+2) = (SQInteger) nrefidx; continue;
				case OT_STRING:
					if((nrefidx = _string(STK(arg0))->Next(STK(arg2+2), STK(arg2), STK(arg2+1))) == -1)break;
					STK(arg2+2) = (SQInteger)nrefidx; continue;
				case OT_USERDATA:
					if(_userdata(STK(arg0))->_delegate) {
						SQObjectPtr itr;
						Push(STK(arg0));
						Push(STK(arg2+2));
						if(CallMetaMethod(_userdata(STK(arg0))->_delegate, MT_NEXTI, 2, itr)){
							STK(arg2+2) = STK(arg2) = itr;
							if(type(itr) == OT_NULL) break;
                            if(!Get(STK(arg0), itr, STK(arg2+1), false))
								RT_Error(_SC("_nexti returned an invalid idx"));
							continue;
						}
					}
				case OT_GENERATOR:
					if(_generator(STK(arg0))->_state == SQGenerator::eDead) break;
					if(_generator(STK(arg0))->_state == SQGenerator::eSuspended) {
						SQInteger idx = 0;
						if(type(STK(arg2+2)) == OT_INTEGER) {
							idx = _integer(STK(arg2+2)) + 1;
						}
						STK(arg2) = idx;
						STK(arg2+2) = idx;
						_generator(STK(arg0))->Resume(this, arg2+1);
						continue;
					}
					default:
					RT_Error(_SC("cannot iterate %s"), GetTypeName(STK(arg0)));
				}
				ci->_ip += sarg1;
				continue;
							 }
			case _OP_DELEGATE:
				if(type(STK(arg1)) != OT_TABLE) RT_Error(_SC("delegating a '%s'"), GetTypeName(STK(arg1)));
				switch(type(STK(arg2))) {
				case OT_TABLE:
					if(!_table(STK(arg1))->SetDelegate(_table(STK(arg2))))
						RT_Error(_SC("delegate cycle detected"));
					break;
				case OT_NULL:
					_table(STK(arg1))->SetDelegate(NULL);
					break;
				default:
					RT_Error(_SC("using '%s' as delegate"), GetTypeName(STK(arg2)));
					break;
				}
				TARGET = STK(arg1);
				continue;
			case _OP_CLONE:
				if(!Clone(STK(arg1), TARGET))
					RT_Error(_SC("cloning a %s"), GetTypeName(STK(arg1)));
				continue;
			case _OP_TYPEOF: TypeOf(STK(arg1), TARGET); continue;
			case _OP_LINE:
				if(type(_debughook) != OT_NULL && _rawval(_debughook) != _rawval(ci->_closure))
					CallDebugHook(_SC('l'),arg1);
				continue;
			case _OP_PUSHTRAP:
				_etraps.push_back(SQExceptionTrap(_top,_stackbase, &ci->_iv->_vals[(ci->_ip-ci->_iv->_vals)+arg1], arg0)); traps++;
				ci->_etraps++;
				continue;
			case _OP_POPTRAP:{
				for(int i=0; i<arg0; i++) {
					_etraps.pop_back(); traps--;
					ci->_etraps--;
				}}
				continue;
			case _OP_THROW:	throw SQException(TARGET); continue;
			}
		}
	}
	SQ_CATCH(SQException,e)
	{
//		dumpstack(_stackbase);
		int n = 0;
		int last_top = _top;
		if(ci) {
			if(traps) {
				do {
					if(ci->_etraps > 0) {
						SQExceptionTrap &et = _etraps.top();
						ci->_ip = et._ip;
						_top = et._stacksize;
						_stackbase = et._stackbase;
						_stack[_stackbase+et._extarget] = e._description;
						_etraps.pop_back(); traps--; ci->_etraps--;
						while(last_top >= _top) _stack[last_top--] = _null_;
						goto exception_restore;
					}
					//if is a native closure
					if(type(ci->_closure) != OT_CLOSURE && n)
						break;
					if(type(ci->_generator) == OT_GENERATOR) _generator(ci->_generator)->Kill();

					POP_CALLINFO(this);
					n++;
				}while(_callsstack.size());
			}

			if(protectedcall) {
				//call the hook
				CallErrorHandler(e);
				//remove call stack until a C function is found or the cstack is empty

				do{
					if(type(ci->_generator) == OT_GENERATOR) _generator(ci->_generator)->Kill();
					_stackbase -= ci->_prevstkbase;
					_top = _stackbase + ci->_prevtop;
					POP_CALLINFO(this);
					if( ci && type(ci->_closure) != OT_CLOSURE) break;
				}while(ci && _callsstack.size() && type(ci->_closure) == OT_CLOSURE);

				while(last_top >= _top) _stack[last_top--] = _null_;
			}
			//thow the exception to terminate the execution of the function
		}
		throw e;
	}
	RT_Error(_SC("internal vm error Execute() returns without executing OP_RETURN"));
}

void SQVM::CallErrorHandler(SQException &e)
{
	if(type(_errorhandler) != OT_NULL) {
		SQObjectPtr out;
		Push(_roottable); Push(e._description);
		Call(_errorhandler, 2, _top-2, out,ET_CALL);
		Pop(2);
	}
}

void SQVM::CallDebugHook(int type,int forcedline)
{
	SQObjectPtr temp_reg;
	int nparams=5;
	SQFunctionProto *func=_funcproto(_closure(ci->_closure)->_function);
	Push(_roottable); Push(type); Push(func->_sourcename); Push(forcedline?forcedline:func->GetLine(ci->_ip)); Push(func->_name);
	Call(_debughook,nparams,_top-nparams,temp_reg,ET_CALL);
	Pop(nparams);
}

bool SQVM::CallNative(SQNativeClosure *nclosure,int nargs,int stackbase,bool tailcall,SQObjectPtr &retval)
{
	if (_nnativecalls + 1 > MAX_NATIVE_CALLS) RT_Error(_SC("Native stack overflow"));
	int nparamscheck = nclosure->_nparamscheck;
	if(((nparamscheck > 0) && (nparamscheck != nargs))
		|| ((nparamscheck < 0) && (nargs < (-nparamscheck)))) 
		RT_Error(_SC("wrong number of parameters"));

	int tcs;
	if(tcs = nclosure->_typecheck.size()) {
		for(int i = 0; i < nargs && i < tcs; i++)
			if((nclosure->_typecheck[i] != -1) && !(type(_stack[stackbase+i]) & nclosure->_typecheck[i]))
					ParamTypeError(i,nclosure->_typecheck[i],type(_stack[stackbase+i]));
	}
	_nnativecalls++;
	if ((_top + MIN_STACK_OVERHEAD) > (int)_stack.size()) {
		_stack.resize(_stack.size() + (MIN_STACK_OVERHEAD<<1));
	}
	int oldtop = _top;
	int oldstackbase = _stackbase;
	_top = stackbase + nargs;
	PUSH_CALLINFO(this, CallInfo());
	ci->_etraps = 0;
	ci->_closure._unVal.pNativeClosure = nclosure;
	ci->_closure._type = OT_NATIVECLOSURE;
	ci->_prevstkbase = stackbase - _stackbase;
	ci->_ncalls = 1;
	_stackbase = stackbase;
	//push outers
	int outers = nclosure->_outervalues.size();
	for (int i = 0; i < outers; i++) {
		Push(nclosure->_outervalues[i]);
	}
	ci->_prevtop = (oldtop - oldstackbase);
	int ret = (nclosure->_function)(this);
	_nnativecalls--;
	bool suspend = false;
	if( ret == SQ_SUSPEND_FLAG) suspend = true;
	else if (ret < 0) RT_Error(_lasterror);
	
	if (ret != 0){ retval = TOP(); }
	else { retval = _null_; }
	_stackbase = oldstackbase;
	_top = oldtop;
	POP_CALLINFO(this);
	return suspend;
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
				Push(self);Push(key);
				if(!CallMetaMethod(_table(self)->_delegate,MT_GET,2,dest)){
					return _table_ddel->Get(key,dest);
				}
			}
			return true;
		}
		if(raw)return false;
		return _table_ddel->Get(key,dest);
		break;
	case OT_ARRAY:
		if(sq_isnumeric(key)){
			return _array(self)->Get(tointeger(key),dest);
		}
		else {
			if(raw)return false;
			return _array_ddel->Get(key,dest);
		}
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
		else {
			if(raw)return false;
			return _string_ddel->Get(key,dest);
		}
		break;
	case OT_USERDATA:{
		bool gret=false;
		if(_userdata(self)->_delegate
		&& !(gret=Get(SQObjectPtr(_userdata(self)->_delegate),key,dest,raw,false))){
			if(raw)return false;
			Push(self);Push(key);
		 	if(!CallMetaMethod(_userdata(self)->_delegate,MT_GET,2,dest)){
				return false;
			}
			return true;
		}
		return gret;
		}
		break;
	case OT_INTEGER:case OT_FLOAT: 
		if(raw)return false;
		return _number_ddel->Get(key,dest);
	case OT_GENERATOR: 
		if(raw)return false;
		return _generator_ddel->Get(key,dest);
	case OT_CLOSURE: case OT_NATIVECLOSURE:	
		if(raw)return false;
		return _closure_ddel->Get(key,dest);
	case OT_THREAD:
		if(raw)return false;
		return  _thread_ddel->Get(key,dest);
	default:RT_Error(_SC("indexing a %s"),GetTypeName(self));return false;
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
		if(!sq_isnumeric(key)) RT_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key));
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
		RT_Error(_SC("trying to set '%s'"),GetTypeName(self));
	}
	return false;

}

bool SQVM::Clone(const SQObjectPtr &self,SQObjectPtr &target)
{
	SQObjectPtr temp_reg;
	switch(type(self)){
	case OT_TABLE:
		target=_table(self)->Clone();
		if(_table(target)->_delegate){
			Push(target);
			Push(self);
			CallMetaMethod(_table(target)->_delegate,MT_CLONED,2,temp_reg);
		}
		return true;
	case OT_ARRAY: 
		target=_array(self)->Clone();
		return true;
	default: return false;
	}
}

void SQVM::NewSlot(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val)
{
	switch(type(self)){
	case OT_TABLE:{
		bool rawcall=true;
		if(type(key)==OT_NULL)RT_Error(_SC("null cannot be used as index"));
		if(_table(self)->_delegate){
			SQObjectPtr res;
			if(!_table(self)->Get(key,res)){
				Push(self);Push(key);Push(val);
				rawcall=!CallMetaMethod(_table(self)->_delegate,MT_NEWSLOT,3,res);
			}
		}
		if(rawcall)_table(self)->NewSlot(key,val);
		break;}
	default:
		RT_Error(_SC("indexing %s with %s"),GetTypeName(self),GetTypeName(key));
		break;
	}
}

void SQVM::DeleteSlot(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &res)
{
	switch(type(self)) {
	case OT_TABLE: {
		SQObjectPtr t;
		bool handled = false;
		if(_table(self)->_delegate) {
			Push(self);Push(key);
			handled = CallMetaMethod(_table(self)->_delegate,MT_DELSLOT,2,t);
		}
		if(!handled) {
			if(_table(self)->Get(key,t))
				_table(self)->Remove(key);
			else
				IdxError((SQObject &)key);
		}
		res = t;
				}
		break;
	case OT_USERDATA:
		if(_userdata(self)->_delegate) {
			Push(self);Push(key);
			SQObjectPtr t;
			if(!CallMetaMethod(_userdata(self)->_delegate,MT_DELSLOT,2,t)) {
				RT_Error(_SC("cannot delete a slot from a userdata"));
			}
			res = t;
		}
		break;
	default:
		RT_Error(_SC("attempt to delete a slot from a %s"),GetTypeName(self));
	}
}

bool SQVM::Call(SQObjectPtr &closure,int nparams,int stackbase,SQObjectPtr &outres,ExecutionType et)
{
	switch(type(closure)) {
	case OT_CLOSURE:
		outres=Execute(closure, _top - nparams, nparams, stackbase,et);
		return true;
	case OT_NATIVECLOSURE: {
		CallNative(_nativeclosure(closure), nparams, stackbase, false, outres); }
		return true;
	default:
		return false;
	}
}

bool SQVM::CallMetaMethod(SQTable *mt,SQMetaMethod mm,int nparams,SQObjectPtr &outres)
{
	SQObjectPtr closure;
	int top = _top;
	if(mt->Get((*_ss(this)->_metamethods)[mm], closure)) {
		if(Call(closure, nparams, _top - nparams, outres,ET_UNPROTECTEDCALL)) {
			Pop(nparams);
			return true;
		}
	}
	assert(top == _top);
	Pop(nparams);
	return false;
}

#ifdef _DEBUG_DUMP
void SQVM::dumpstack(int stackbase,bool dumpall)
{
	int size=dumpall?_stack.size():_top;
	int n=0;
	scprintf(_SC("\n>>>>stack dump<<<<\n"));
	CallInfo &ci=_callsstack.back();
	scprintf(_SC("IP: %d\n"),ci._ip);
	scprintf(_SC("prev stack base: %d\n"),ci._prevstkbase);
	scprintf(_SC("prev top: %d\n"),ci._prevtop);
	for(int i=0;i<size;i++){
		SQObjectPtr &obj=_stack[i];	
		if(stackbase==i)scprintf(_SC(">"));else scprintf(_SC(" "));
		scprintf(_SC("[%d]:"),n);
		switch(type(obj)){
		case OT_FLOAT:			scprintf(_SC("FLOAT %.3f"),_float(obj));break;
		case OT_INTEGER:		scprintf(_SC("INTEGER %d"),_integer(obj));break;
		case OT_STRING:			scprintf(_SC("STRING %s"),_stringval(obj));break;
		case OT_NULL:			scprintf(_SC("NULL"));	break;
		case OT_TABLE:			scprintf(_SC("TABLE %p[%p]"),_table(obj),_table(obj)->_delegate);break;
		case OT_ARRAY:			scprintf(_SC("ARRAY %p"),_array(obj));break;
		case OT_CLOSURE:		scprintf(_SC("CLOSURE [%p]"),_closure(obj));break;
		case OT_NATIVECLOSURE:	scprintf(_SC("NATIVECLOSURE"));break;
		case OT_USERDATA:		scprintf(_SC("USERDATA %p[%p]"),_userdataval(obj),_userdata(obj)->_delegate);break;
		case OT_GENERATOR:		scprintf(_SC("GENERATOR"));break;
		case OT_THREAD:			scprintf(_SC("THREAD [%p]"),_thread(obj));break;
		case OT_USERPOINTER:	scprintf(_SC("USERPOINTER %p"),_userpointer(obj));break;
		default:
			assert(0);
			break;
		};
		scprintf(_SC("\n"));
		++n;
	}
}

#endif
