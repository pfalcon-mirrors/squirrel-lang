/*
	see copyright notice in squirrel.h
*/
#include "sqpcheader.h"
#include "sqvm.h"
#include "sqstring.h"
#include "sqarray.h"
#include "sqtable.h"
#include "squserdata.h"
#include "sqfuncproto.h"

#include "sqclosure.h"

SQString *SQString::Create(SQSharedState *ss,const SQChar *s,int len)
{
	SQString *str=ADD_STRING(ss,s,len);
	str->_sharedstate=ss;
	return str;
}

void SQString::Release()
{
	REMOVE_STRING(_sharedstate,this);
}

int SQGenerator::Yield(SQVM *v)
{
	if(_state==eSuspended)sqraise_str_error(_ss(v),"internal vm error, yielding dead generator");
	if(_state==eDead)sqraise_str_error(_ss(v),"internal vm error, yielding a dead generator");
	int size=v->_top-v->_stackbase;
	_ci=*v->ci;
	_stack.resize(size);
	int bytesize=sizeof(SQObjectPtr)*size;
	memcpy(&_stack._vals[0],&v->_stack[v->_stackbase],bytesize);
	memset(&v->_stack[v->_stackbase],0,bytesize);
	_ci._generator=_null_;
	_state=eSuspended;
	return size;
}

void SQGenerator::Resume(SQVM *v,int target)
{
	int size=_stack.size();
	if(_state==eDead)sqraise_str_error(_ss(v),"resuming dead generator");
	if(_state==eRunning)sqraise_str_error(_ss(v),"resuming active generator");
	int prevtop=v->_top-v->_stackbase;
	PUSH_CALLINFO(v,_ci);
	int oldstackbase=v->_stackbase;
	v->_stackbase=v->_top;
	v->ci->_target=target;
	v->ci->_generator=this;
	int bytesize=sizeof(SQObjectPtr)*size;
	memcpy(&v->_stack[v->_stackbase],&_stack._vals[0],bytesize);
	memset(&_stack._vals[0],0,bytesize);
	v->_top=v->_stackbase+size;
	v->ci->_prevtop=prevtop;
	v->ci->_prevstkbase=v->_stackbase-oldstackbase;
	_state=eRunning;
}

const SQChar* SQFunctionProto::GetLocal(SQVM *vm,unsigned int stackbase,unsigned int nseq,unsigned int nop)
{
	unsigned int nvars=_localvarinfos.size();
	const SQChar *res=NULL; 
	if(nvars>=nseq){
 		for(unsigned int i=0;i<nvars;i++){
			if(_localvarinfos[i]._start_op<=nop && _localvarinfos[i]._end_op>=nop)
			{
				if(nseq==0){
					vm->Push(vm->_stack[stackbase+_localvarinfos[i]._pos]);
					res=_stringval(_localvarinfos[i]._name);
					break;
				}
				nseq--;
			}
		}
	}
	return res;
}

int SQFunctionProto::GetLine(SQInstruction *curr)
{
	int op=(curr-_instructions._vals)-1;
	int line=1;
	for(unsigned int i=0;i<_lineinfos.size();i++){
		if(_lineinfos[i]._op>=op)
			return line;
		line=_lineinfos[i]._line;
	}
	return -1;
}

int WriteObject(HSQUIRRELVM v,SQUserPointer up,SQWRITEFUNC write,SQObjectPtr &o)
{
	int nwritten=write(up,&type(o),sizeof(SQObjectType));
	switch(type(o)){
	case OT_STRING:
		nwritten+=write(up,&_string(o)->_len,4);
		nwritten+=write(up,_stringval(o),_string(o)->_len);
		break;
	case OT_INTEGER:
		nwritten+=write(up,&_integer(o),sizeof(SQInteger));break;
	case OT_FLOAT:
		nwritten+=write(up,&_float(o),sizeof(SQFloat));break;
	case OT_NULL:
		break;
	default:
		assert(0);
	}
	return nwritten;
}

int ReadObject(HSQUIRRELVM v,SQUserPointer up,SQREADFUNC read,SQObjectPtr &o)
{
	SQObjectType t;
	int nreaded=read(up,&t,sizeof(SQObjectType));
	switch(t){
	case OT_STRING:{
		int len;
		nreaded+=read(up,&len,sizeof(int));
		nreaded+=read(up,_ss(v)->GetScratchPad(len),len);
		o=SQString::Create(_ss(v),_ss(v)->GetScratchPad(-1),len);
				   }
		break;
	case OT_INTEGER:{
		SQInteger i;
		nreaded+=read(up,&i,sizeof(SQInteger)); o = i; break;
					}
	case OT_FLOAT:{
		SQFloat f;
		nreaded+=read(up,&f,sizeof(SQFloat)); o = f; break;
				  }
	case OT_NULL:
		o=_null_;
		break;
	default:
		assert(0);
	}
	return nreaded;
}

void SQClosure::Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write)
{
	_funcproto(_function)->Save(v,up,write);
}

void SQClosure::Load(SQVM *v,SQUserPointer up,SQREADFUNC read)
{
	_function=SQFunctionProto::Create();
	_funcproto(_function)->Load(v,up,read);
}

void SQFunctionProto::Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write)
{
	int i,nsize=_literals.size();
	WriteObject(v,up,write,_sourcename);
	WriteObject(v,up,write,_name);
	write(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		WriteObject(v,up,write,_literals[i]);
	}
	nsize=_parameters.size();
	write(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		WriteObject(v,up,write,_parameters[i]);
	}
	nsize=_outervalues.size();
	write(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		write(up,&_outervalues[i]._blocal,sizeof(bool));
		WriteObject(v,up,write,_outervalues[i]._src);
	}
	nsize=_localvarinfos.size();
	write(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		SQLocalVarInfo &lvi=_localvarinfos[i];
		WriteObject(v,up,write,lvi._name);
		write(up,&lvi._pos,sizeof(unsigned int));
		write(up,&lvi._start_op,sizeof(unsigned int));
		write(up,&lvi._end_op,sizeof(unsigned int));
	}
	nsize=_lineinfos.size();
	write(up,&nsize,sizeof(nsize));
	write(up,&_lineinfos[0],sizeof(SQLineInfo)*nsize);
	nsize=_instructions.size();
	write(up,&nsize,sizeof(nsize));
	write(up,&_instructions[0],sizeof(SQInstruction)*nsize);
	nsize=_functions.size();
	write(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		_funcproto(_functions[i])->Save(v,up,write);
	}
	write(up,&_stacksize,sizeof(_stacksize));
	write(up,&_bgenerator,sizeof(_bgenerator));
}

void SQFunctionProto::Load(SQVM *v,SQUserPointer up,SQREADFUNC read)
{
	int i,nsize=_literals.size();
	SQObjectPtr o;
	ReadObject(v,up,read,_sourcename);
	ReadObject(v,up,read,_name);
	read(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		ReadObject(v,up,read,o);
		_literals.push_back(o);
	}
	read(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		ReadObject(v,up,read,o);
		_parameters.push_back(o);
	}
	read(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		bool bl;
		read(up,&bl,sizeof(bool));
		ReadObject(v,up,read,o);
		_outervalues.push_back(SQOuterVar(o,bl));
	}
	read(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		SQLocalVarInfo lvi;
		ReadObject(v,up,read,lvi._name);
		read(up,&lvi._pos,sizeof(unsigned int));
		read(up,&lvi._start_op,sizeof(unsigned int));
		read(up,&lvi._end_op,sizeof(unsigned int));
		_localvarinfos.push_back(lvi);
	}
	read(up,&nsize,sizeof(nsize));
	_lineinfos.resize(nsize);
	read(up,&_lineinfos[0],sizeof(SQLineInfo)*nsize);
	read(up,&nsize,sizeof(nsize));
	_instructions.resize(nsize);
	read(up,&_instructions[0],sizeof(SQInstruction)*nsize);
	read(up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		o=SQFunctionProto::Create();
		_funcproto(o)->Load(v,up,read);
		_functions.push_back(o);
	}
	read(up,&_stacksize,sizeof(_stacksize));
	read(up,&_bgenerator,sizeof(_bgenerator));
}

#ifdef GARBAGE_COLLECTOR

#define START_MARK() 	if(!(_uiRef&MARK_FLAG)){ \
		_uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain,this); \
		AddToChain(chain,this); }

void SQArray::Mark(SQCollectable **chain)
{
	START_MARK()
		int len=_values.size();
		for(int i=0;i<len;i++)SQSharedState::MarkObject(_values[i],chain);
	END_MARK()
}
void SQTable::Mark(SQCollectable **chain)
{
	START_MARK()
		int len=_numofnodes;
		for(int i=0;i<len;i++){
			SQSharedState::MarkObject(_nodes[i].key,chain);
			SQSharedState::MarkObject(_nodes[i].val,chain);
		}
	END_MARK()
}

void SQGenerator::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i=0;i<_stack.size();i++)SQSharedState::MarkObject(_stack[i],chain);
		SQSharedState::MarkObject(_closure,chain);
	END_MARK()
}

void SQClosure::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i=0;i<_outervalues.size();i++)SQSharedState::MarkObject(_outervalues[i],chain);
	END_MARK()
}

void SQNativeClosure::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i=0;i<_outervalues.size();i++)SQSharedState::MarkObject(_outervalues[i],chain);
	END_MARK()
}

void SQUserData::Mark(SQCollectable **chain){
	if(_delegate)_delegate->Mark(chain);
	RemoveFromChain(&_sharedstate->_gc_chain,this);
	AddToChain(chain,this);
}

void SQCollectable::UnMark(){_uiRef&=~MARK_FLAG;}

#endif
