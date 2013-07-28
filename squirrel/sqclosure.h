/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

struct SQFunctionProto;
struct SQClosure : public CHAINABLE_OBJ
{
private:
	SQClosure(SQSharedState *ss,SQFunctionProto *func){_uiRef=0;_function=func;_bgenerator=func->_bgenerator; INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQClosure *Create(SQSharedState *ss,SQFunctionProto *func){
		SQClosure *nc=(SQClosure*)SQ_MALLOC(sizeof(SQClosure));
		new (nc) SQClosure(ss,func);
		
		return nc;
	}
	void Release(){
		sq_delete(this,SQClosure);
	}
	~SQClosure()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	void Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write);
	void Load(SQVM *v,SQUserPointer up,SQREADFUNC read);
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	void Clear(){_outervalues.resize(0);}
#endif
	SQObjectPtr _function;
	SQObjectPtrVec _outervalues;
	bool _bgenerator;
};
//////////////////////////////////////////////
struct SQGenerator : public CHAINABLE_OBJ 
{
	enum SQGEneratorState{eRunning,eSuspended,eDead};
private:
	SQGenerator(SQSharedState *ss,SQClosure *closure){_uiRef=0;_closure=closure;_state=eRunning;_ci._generator=_null_;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);}
public:
	static SQGenerator *Create(SQSharedState *ss,SQClosure *closure){
		SQGenerator *nc=(SQGenerator*)SQ_MALLOC(sizeof(SQGenerator));
		new (nc) SQGenerator(ss,closure);
		return nc;
	}
	~SQGenerator()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
    void Kill(){
		_state=eDead;
		_stack.resize(0);
		_closure=_null_;}
	void Release(){
		sq_delete(this,SQGenerator);
	}
	int Yield(SQVM *v);
	void Resume(SQVM *v,int target);
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	void Clear(){_stack.resize(0);_closure=_null_;}
#endif
	SQObjectPtr _closure;
	SQObjectPtrVec _stack;
	SQVM::CallInfo _ci;
	SQGEneratorState _state;
};

struct SQNativeClosure : public CHAINABLE_OBJ
{
private:
	SQNativeClosure(SQSharedState *ss,SQFUNCTION func){_uiRef=0;_function=func;INIT_CHAIN();ADD_TO_CHAIN(&_ss(this)->_gc_chain,this);	}
public:
	static SQNativeClosure *Create(SQSharedState *ss,SQFUNCTION func)
	{
		SQNativeClosure *nc=(SQNativeClosure*)SQ_MALLOC(sizeof(SQNativeClosure));
		new (nc) SQNativeClosure(ss,func);
		
		return nc;
	}
	~SQNativeClosure()
	{
		REMOVE_FROM_CHAIN(&_ss(this)->_gc_chain,this);
	}
	void Release(){
		sq_delete(this,SQNativeClosure);
	}
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	void Clear(){_outervalues.resize(0);}
#endif
	SQFUNCTION _function;
	SQObjectPtrVec _outervalues;
};



#endif //_SQCLOSURE_H_
