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

unsigned int TranslateIndex(const SQObjectPtr &idx)
{
	switch(type(idx)){
		case OT_NULL:
			return 0;
		case OT_INTEGER:
			return (unsigned int)_integer(idx);
	}
	assert(0);
	return 0;
}

int SQGenerator::Yield(SQVM *v)
{
	if(_state==eSuspended)v->RT_Error(_SC("internal vm error, yielding dead generator"));
	if(_state==eDead)v->RT_Error(_SC("internal vm error, yielding a dead generator"));
	int size=v->_top-v->_stackbase;
	_ci=*v->ci;
	_stack.resize(size);
	int bytesize=sizeof(SQObjectPtr)*size;
	memcpy(&_stack._vals[0],&v->_stack[v->_stackbase],bytesize);
	memset(&v->_stack[v->_stackbase],0,bytesize);
	_ci._generator=_null_;
	for(int i=0;i<_ci._etraps;i++) {
		_etraps.push_back(v->_etraps.top());
		v->_etraps.pop_back();
	}
	_state=eSuspended;
	return size;
}

void SQGenerator::Resume(SQVM *v,int target)
{
	int size=_stack.size();
	if(_state==eDead)v->RT_Error(_SC("resuming dead generator"));
	if(_state==eRunning)v->RT_Error(_SC("resuming active generator"));
	int prevtop=v->_top-v->_stackbase;
	PUSH_CALLINFO(v,_ci);
	int oldstackbase=v->_stackbase;
	v->_stackbase=v->_top;
	v->ci->_target=target;
	v->ci->_generator=SQObjectPtr(this);
	for(int i=0;i<_ci._etraps;i++) {
		v->_etraps.push_back(_etraps.top());
		_etraps.pop_back();
	}
    int bytesize=sizeof(SQObjectPtr)*size;
	memcpy(&v->_stack[v->_stackbase],&_stack._vals[0],bytesize);
	memset(&_stack._vals[0],0,bytesize);
	v->_top=v->_stackbase+size;
	v->ci->_prevtop=prevtop;
	v->ci->_prevstkbase=v->_stackbase-oldstackbase;
	_state=eRunning;
}

void SQArray::Extend(const SQArray *a){
	int xlen;
	if((xlen=a->Size()))
		for(int i=0;i<xlen;i++)
			Append(a->_values[i]);
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
	int op=(curr-_instructions._vals)+1;
	int line=_lineinfos[0]._line;
	for(unsigned int i=1;i<_lineinfos.size();i++){
		if(_lineinfos[i]._op>=op)
			return line;
		line=_lineinfos[i]._line;
	}
	return line;
}

int SafeWrite(HSQUIRRELVM v,SQWRITEFUNC write,SQUserPointer up,SQUserPointer dest,int size)
{
	if(write(up,dest,size) != size)
		v->RT_Error(_SC("io error (write function failure)"));
	return size;
}

int SafeRead(HSQUIRRELVM v,SQWRITEFUNC read,SQUserPointer up,SQUserPointer dest,int size)
{
	if(size && read(up,dest,size) != size)
		v->RT_Error(_SC("io error, read function failure, the origin stream could be corrupted/trucated"));
	return size;
}

int WriteTag(HSQUIRRELVM v,SQWRITEFUNC write,SQUserPointer up,int tag)
{
	return SafeWrite(v,write,up,&tag,sizeof(tag));
}

int CheckTag(HSQUIRRELVM v,SQWRITEFUNC read,SQUserPointer up,int tag)
{
	int t,ret = SafeRead(v,read,up,&t,sizeof(t));
	if(t != tag)
		v->RT_Error(_SC("invalid or corrupted closure stream"));
	return ret;
}

int WriteObject(HSQUIRRELVM v,SQUserPointer up,SQWRITEFUNC write,SQObjectPtr &o)
{
	int nwritten=SafeWrite(v,write,up,&type(o),sizeof(SQObjectType));
	switch(type(o)){
	case OT_STRING:
		nwritten+=SafeWrite(v,write,up,&_string(o)->_len,sizeof(SQInteger));
		nwritten+=SafeWrite(v,write,up,_stringval(o),rsl(_string(o)->_len));
		break;
	case OT_INTEGER:
		nwritten+=SafeWrite(v,write,up,&_integer(o),sizeof(SQInteger));break;
	case OT_FLOAT:
		nwritten+=SafeWrite(v,write,up,&_float(o),sizeof(SQFloat));break;
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
	int nreaded=SafeRead(v,read,up,&t,sizeof(SQObjectType));
	switch(t){
	case OT_STRING:{
		int len;
		nreaded+=SafeRead(v,read,up,&len,sizeof(SQInteger));
		nreaded+=SafeRead(v,read,up,_ss(v)->GetScratchPad(rsl(len)),rsl(len));
		o=SQString::Create(_ss(v),_ss(v)->GetScratchPad(-1),len);
				   }
		break;
	case OT_INTEGER:{
		SQInteger i;
		nreaded+=SafeRead(v,read,up,&i,sizeof(SQInteger)); o = i; break;
					}
	case OT_FLOAT:{
		SQFloat f;
		nreaded+=SafeRead(v,read,up,&f,sizeof(SQFloat)); o = f; break;
				  }
	case OT_NULL:
		o=_null_;
		break;
	default:
		v->RT_Error(_SC("cannot serialize a %s"),GetTypeName(t));
	}
	return nreaded;
}

void SQClosure::Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write)
{
	WriteTag(v,write,up,SQ_CLOSURESTREAM_HEAD);
	WriteTag(v,write,up,sizeof(SQChar));
	_funcproto(_function)->Save(v,up,write);
	WriteTag(v,write,up,SQ_CLOSURESTREAM_TAIL);
}

void SQClosure::Load(SQVM *v,SQUserPointer up,SQREADFUNC read)
{
	CheckTag(v,read,up,SQ_CLOSURESTREAM_HEAD);
	CheckTag(v,read,up,sizeof(SQChar));
	_funcproto(_function)->Load(v,up,read);
	CheckTag(v,read,up,SQ_CLOSURESTREAM_TAIL);
}

void SQFunctionProto::Save(SQVM *v,SQUserPointer up,SQWRITEFUNC write)
{
	int i,nsize=_literals.size();
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	WriteObject(v,up,write,_sourcename);
	WriteObject(v,up,write,_name);
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		WriteObject(v,up,write,_literals[i]);
	}
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_parameters.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		WriteObject(v,up,write,_parameters[i]);
	}
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_outervalues.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		SafeWrite(v,write,up,&_outervalues[i]._blocal,sizeof(bool));
		WriteObject(v,up,write,_outervalues[i]._src);
	}
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_localvarinfos.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		SQLocalVarInfo &lvi=_localvarinfos[i];
		WriteObject(v,up,write,lvi._name);
		SafeWrite(v,write,up,&lvi._pos,sizeof(unsigned int));
		SafeWrite(v,write,up,&lvi._start_op,sizeof(unsigned int));
		SafeWrite(v,write,up,&lvi._end_op,sizeof(unsigned int));
	}
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_lineinfos.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	SafeWrite(v,write,up,&_lineinfos[0],sizeof(SQLineInfo)*nsize);
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_instructions.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	SafeWrite(v,write,up,&_instructions[0],sizeof(SQInstruction)*nsize);
	WriteTag(v,write,up,SQ_CLOSURESTREAM_PART);
	nsize=_functions.size();
	SafeWrite(v,write,up,&nsize,sizeof(nsize));
	for(i=0;i<nsize;i++){
		_funcproto(_functions[i])->Save(v,up,write);
	}
	SafeWrite(v,write,up,&_stacksize,sizeof(_stacksize));
	SafeWrite(v,write,up,&_bgenerator,sizeof(_bgenerator));
}

void SQFunctionProto::Load(SQVM *v,SQUserPointer up,SQREADFUNC read)
{
	int i, nsize = _literals.size();
	SQObjectPtr o;
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	ReadObject(v, up, read, _sourcename);
	ReadObject(v, up, read, _name);
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up, &nsize, sizeof(nsize));
	for(i = 0;i < nsize; i++){
		ReadObject(v, up, read, o);
		_literals.push_back(o);
	}
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up, &nsize, sizeof(nsize));
	for(i = 0; i < nsize; i++){
		ReadObject(v, up, read, o);
		_parameters.push_back(o);
	}
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up,&nsize,sizeof(nsize));
	for(i = 0; i < nsize; i++){
		bool bl;
		SafeRead(v,read,up, &bl, sizeof(bool));
		ReadObject(v, up, read, o);
		_outervalues.push_back(SQOuterVar(o, bl));
	}
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up,&nsize, sizeof(nsize));
	for(i = 0; i < nsize; i++){
		SQLocalVarInfo lvi;
		ReadObject(v, up, read, lvi._name);
		SafeRead(v,read,up, &lvi._pos, sizeof(unsigned int));
		SafeRead(v,read,up, &lvi._start_op, sizeof(unsigned int));
		SafeRead(v,read,up, &lvi._end_op, sizeof(unsigned int));
		_localvarinfos.push_back(lvi);
	}
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up, &nsize,sizeof(nsize));
	_lineinfos.resize(nsize);
	SafeRead(v,read,up, &_lineinfos[0], sizeof(SQLineInfo)*nsize);
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up, &nsize, sizeof(nsize));
	_instructions.resize(nsize);
	SafeRead(v,read,up, &_instructions[0], sizeof(SQInstruction)*nsize);
	CheckTag(v,read,up,SQ_CLOSURESTREAM_PART);
	SafeRead(v,read,up, &nsize, sizeof(nsize));
	for(i = 0; i < nsize; i++){
		o = SQFunctionProto::Create();
		_funcproto(o)->Load(v, up, read);
		_functions.push_back(o);
	}
	SafeRead(v,read,up, &_stacksize, sizeof(_stacksize));
	SafeRead(v,read,up, &_bgenerator, sizeof(_bgenerator));
}

#ifndef NO_GARBAGE_COLLECTOR

#define START_MARK() 	if(!(_uiRef&MARK_FLAG)){ \
		_uiRef|=MARK_FLAG;

#define END_MARK() RemoveFromChain(&_sharedstate->_gc_chain, this); \
		AddToChain(chain, this); }

void SQVM::Mark(SQCollectable **chain)
{
	START_MARK()
		SQSharedState::MarkObject(_lasterror,chain);
		SQSharedState::MarkObject(_errorhandler,chain);
		SQSharedState::MarkObject(_debughook,chain);
		SQSharedState::MarkObject(_roottable, chain);
		SQSharedState::MarkObject(temp_reg, chain);
		for(unsigned int i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
	END_MARK()
}

void SQArray::Mark(SQCollectable **chain)
{
	START_MARK()
		int len = _values.size();
		for(int i = 0;i < len; i++) SQSharedState::MarkObject(_values[i], chain);
	END_MARK()
}
void SQTable::Mark(SQCollectable **chain)
{
	START_MARK()
		if(_delegate) _delegate->Mark(chain);
		int len = _numofnodes;
		for(int i = 0; i < len; i++){
			SQSharedState::MarkObject(_nodes[i].key, chain);
			SQSharedState::MarkObject(_nodes[i].val, chain);
		}
	END_MARK()
}

void SQGenerator::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i = 0; i < _stack.size(); i++) SQSharedState::MarkObject(_stack[i], chain);
		SQSharedState::MarkObject(_closure, chain);
	END_MARK()
}

void SQClosure::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i = 0; i < _outervalues.size(); i++) SQSharedState::MarkObject(_outervalues[i], chain);
	END_MARK()
}

void SQNativeClosure::Mark(SQCollectable **chain)
{
	START_MARK()
		for(unsigned int i = 0; i < _outervalues.size(); i++) SQSharedState::MarkObject(_outervalues[i], chain);
	END_MARK()
}

void SQUserData::Mark(SQCollectable **chain){
	START_MARK()
		if(_delegate) _delegate->Mark(chain);
	END_MARK()
}

void SQCollectable::UnMark() { _uiRef&=~MARK_FLAG; }

#endif
