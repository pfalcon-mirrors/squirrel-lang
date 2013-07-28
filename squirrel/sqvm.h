/*	see copyright notice in squirrel.h */
#ifndef _SQVM_H_
#define _SQVM_H_

#include "sqopcodes.h"
#include "sqobject.h"
#define MAX_NATIVE_CALLS 100
#define MIN_STACK_OVERHEAD 10
//base lib
void sq_base_register(HSQUIRRELVM v);

struct SQExceptionTrap{
	SQExceptionTrap(){}
	SQExceptionTrap(int ss,int stackbase,SQInstruction *ip,int ex_target){_stacksize=ss;_stackbase=stackbase;_ip=ip;_extarget=ex_target;}
	SQExceptionTrap(const SQExceptionTrap &et)
	{
		_stacksize=et._stacksize;
		_stackbase=et._stackbase;
		_ip=et._ip;
		_extarget=et._extarget;
	}
	int _stackbase;
	int _stacksize;
	SQInstruction *_ip;
	int _extarget;
};

typedef sqvector<SQExceptionTrap> ExceptionsTraps;
struct SQVM 
{
	struct CallInfo{
		CallInfo(){}
		CallInfo(const CallInfo& ci){(*this)=ci;}
		SQInstructionVec *_iv;
		SQObjectPtrVec *_literals;
		SQObject _closure;
		SQObject _generator;
		ExceptionsTraps _etraps;
		int _prevstkbase;
		int _prevtop;
		int _target;
		SQInstruction *_ip;
		bool _root;
	};

typedef sqvector<CallInfo> CallInfoVec;
public:
	SQVM(SQSharedState *ss);
	~SQVM();
	bool Init(int stacksize);
	SQObjectPtr Execute(SQObjectPtr &func,int target,int nargs,int stackbase,bool bresume=false);
	//start a native call return when the NATIVE closure returns
	SQObjectPtr CallNative(SQObjectPtr &nclosure,int nargs,int stackbase,bool tailcall);
	//start a SQUIRREL call in the same "Execution loop"
	void StartCall(SQObjectPtr &closure,int target,int nargs,int stackbase,bool tailcall);
	//call a generic closure pure SQUIRREL or NATIVE
	bool Call(SQObjectPtr &closure,int nparams,int stackbase,SQObjectPtr &outres);

	void CallDebugHook(int type);
	SQObjectPtrVec _stack;
	int _top;
	int _stackbase;
	SQObjectPtr _roottable;
		
	bool Get(const SQObjectPtr &self,const SQObjectPtr &key,SQObjectPtr &dest,bool raw,bool root=true);
	bool Set(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
	void NewSlot(const SQObjectPtr &self,const SQObjectPtr &key,const SQObjectPtr &val);
	bool Clone(const SQObjectPtr &self,SQObjectPtr &target);
	int ObjCmp(const SQObjectPtr &o1,const SQObjectPtr &o2);
	void StringCat(const SQObjectPtr &str,const SQObjectPtr &obj,SQObjectPtr &dest);
	void IdxError(SQObject &o);
	void CompareError(const SQObject &o1,const SQObject &o2);
	void RT_Error(const SQChar *s,...);
	SQString *PrintObjVal(const SQObject &o);

	void TypeOf(const SQObjectPtr &obj1,SQObjectPtr &dest);
	bool CallMetaMethod(SQTable *mt,SQMetaMethod mm,int nparams,SQObjectPtr &outres);
	bool ArithMetaMethod(int op,const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest);
	void Modulo(const SQObjectPtr &o1,const SQObjectPtr &o2,SQObjectPtr &dest);
	bool Return(int _arg0,int _arg1,SQObjectPtr &retval);

	//void LocalInc(SQObjectPtr &target,SQObjectPtr &a,SQObjectPtr &incr,bool postfix,bool reassing);
	void DerefInc(SQObjectPtr &target,SQObjectPtr &self,SQObjectPtr &key,SQObjectPtr &incr,bool postfix);
#ifdef _DEBUG
	void dumpstack(int stackbase=-1,bool dumpall=false);
#endif
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
	void UnMark(){}; //does nothing
#endif
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	void Clear(){}; //does nothing
	void Release(){delete this;} //does nothing
#endif
////////////////////////////////////////////////////////////////////////////
	//stack functions for he api
	void Pop(){
		_stack[--_top]=_null_;
	}
	void Pop(int n){
		for(int i=0;i<n;i++){
			_stack[--_top]=_null_;
		}
	}
	void Remove(int n){
		n=(n>=0)?n+_stackbase-1:_top+n;
		for(int i=n;i<_top;i++){
			_stack[i]=_stack[i+1];
		}
		_stack[_top]=_null_;
		_top--;
	}


	void Push(const SQObjectPtr &o){_stack[_top++]=o;}
	SQObjectPtr &Top(){return _stack[_top-1];}
	SQObjectPtr &PopGet(){return _stack[--_top];}
	SQObjectPtr &GetUp(int n){return _stack[_top+n];}
	SQObjectPtr &GetAt(int n){return _stack[n];}
	SQObjectPtr _lasterror;
	SQObjectPtr _errorhandler;
	SQObjectPtr _debughook;
	SQObjectPtr _refs_table;
	SQObjectPtr temp;
	CallInfoVec _callsstack;
	CallInfo *ci;
	SQCOMPILERERROR _compilererrorhandler;
	void *_foreignptr;
	//VMs sharing the same state
	SQVM *_next;
	SQVM *_prev;
	SQSharedState *_sharedstate;
	int _nnativecalls;
};

struct AutoDec{
	AutoDec(int *n){_n=n;}
	~AutoDec(){(*_n)--;}
	int *_n;
};

SQObjectPtr &stack_get(HSQUIRRELVM v,int idx);
const SQChar *GetTypeName(const SQObjectPtr &obj1);
const SQChar *GetTypeName(SQObjectType type);

#define _ss(_vm_) (_vm_)->_sharedstate

#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
#define _opt_ss(_vm_) (_vm_)->_sharedstate
#else
#define _opt_ss(_vm_) NULL
#endif

#define PUSH_CALLINFO(v,nci){ \
	v->_callsstack.push_back(nci); \
	v->ci=&v->_callsstack.back(); \
}

#define POP_CALLINFO(v){ \
	v->_callsstack.pop_back(); \
	if(v->_callsstack.size())	\
		v->ci=&v->_callsstack.back() ; \
	else	\
		v->ci=NULL ; \
}
#endif //_SQVM_H_
