/*	see copyright notice in squirrel.h */
#ifndef _SQCLOSURE_H_
#define _SQCLOSURE_H_

struct SQFunctionProto;
struct SQClosure : public CHAINABLE_OBJ
{
private:
	SQClosure(SQFunctionProto *func){_uiRef=0;_function=func;_bgenerator=func->_bgenerator; INIT_CHAIN();}
public:
	static SQClosure *Create(SQFunctionProto *func){
		SQClosure *nc=(SQClosure*)SQ_MALLOC(sizeof(SQClosure));
		new (nc) SQClosure(func);
		ADD_TO_CHAIN(&GS->_gc_chain,nc);
		return nc;
	}
	void Release(){
		sq_delete(this,SQClosure);
	}
	~SQClosure()
	{
		REMOVE_FROM_CHAIN(&GS->_gc_chain,this);
	}
	void Save(SQUserPointer up,SQWRITEFUNC write);
	void Load(SQUserPointer up,SQREADFUNC read);
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
	SQGenerator(SQClosure *closure){_uiRef=0;_closure=closure;_state=eRunning;_ci._generator=_null_;INIT_CHAIN();}
public:
	static SQGenerator *Create(SQClosure *closure){
		SQGenerator *nc=(SQGenerator*)SQ_MALLOC(sizeof(SQGenerator));
		new (nc) SQGenerator(closure);
		ADD_TO_CHAIN(&GS->_gc_chain,nc);
		return nc;
	}
	~SQGenerator()
	{
		REMOVE_FROM_CHAIN(&GS->_gc_chain,this);
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
	SQNativeClosure(SQFUNCTION func){_uiRef=0;_function=func;INIT_CHAIN();	}
public:
	static SQNativeClosure *Create(SQFUNCTION func)
	{
		SQNativeClosure *nc=(SQNativeClosure*)SQ_MALLOC(sizeof(SQNativeClosure));
		new (nc) SQNativeClosure(func);
		ADD_TO_CHAIN(&GS->_gc_chain,nc);
		return nc;
	}
	~SQNativeClosure()
	{
		REMOVE_FROM_CHAIN(&GS->_gc_chain,this);
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
