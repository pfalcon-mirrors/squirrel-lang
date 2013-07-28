/*
	see copyright notice in squirrel.h
*/
#include "stdafx.h"
#include "sqopcodes.h"
#include "sqvm.h"
#include "sqfuncproto.h"
#include "sqclosure.h"
#include "sqstring.h"
#include "sqtable.h"
#include "sqarray.h"
#include "squserdata.h"

SQObjectPtr _null_;
SQObjectPtr _notnull_(1);

SQGlobalState *GS=NULL;
SQGlobalState::SQGlobalState()
{
}

#define newsysstring(s) {	\
	_systemstrings->push_back(SQString::Create(s));	\
	}

#define newmetamethod(s) {	\
	_metamethods->push_back(SQString::Create(s));	\
	}

SQTable *CreateDefaultDelegate(SQRegFunction *funcz)
{
	int i=0;
	SQTable *t=SQTable::Create(0);
	while(funcz[i].name!=0){
		t->NewSlot(SQString::Create(funcz[i].name),SQNativeClosure::Create(funcz[i].f));
		i++;
	}
	return t;
}

void SQGlobalState::Init(SQUIRREL_MALLOC _malloc,SQUIRREL_REALLOC _realloc,SQUIRREL_FREE _free)
{
	_scratchpad=NULL;
	_scratchpadsize=0;
	_sq_malloc=_malloc;
	_sq_realloc=_realloc;
	_sq_free=_free;
	_notnull=1;
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	_gc_chain=NULL;_vms_chain=NULL;
#endif
	sq_new(_stringtable,StringTable);
	sq_new(_metamethods,SQObjectPtrVec);
	sq_new(_systemstrings,SQObjectPtrVec);
	sq_new(_types,SQObjectPtrVec);
	//adding type strings to avoid memory trashing
	//types names
	newsysstring("null");
	newsysstring("table");
	newsysstring("array");
	newsysstring("closure");
	newsysstring("string");
	newsysstring("userdata");
	newsysstring("integer");
	newsysstring("float");
	newsysstring("userpointer");
	newsysstring("function");
	newsysstring("generator");
	//meta methods
	newmetamethod(MM_ADD);
	newmetamethod(MM_SUB);
	newmetamethod(MM_MUL);
	newmetamethod(MM_DIV);
	newmetamethod(MM_UNM);
	newmetamethod(MM_MODULO);
	newmetamethod(MM_SET);
	newmetamethod(MM_GET);
	newmetamethod(MM_TYPEOF);
	newmetamethod(MM_NEXTI);
	newmetamethod(MM_CMP);
	newmetamethod(MM_CALL);
	newmetamethod(MM_CLONE);

	_table_default_delegate=CreateDefaultDelegate(_table_default_delegate_funcz);
	_array_default_delegate=CreateDefaultDelegate(_array_default_delegate_funcz);
	_string_default_delegate=CreateDefaultDelegate(_string_default_delegate_funcz);
	_number_default_delegate=CreateDefaultDelegate(_number_default_delegate_funcz);
	_closure_default_delegate=CreateDefaultDelegate(_closure_default_delegate_funcz);
	_generator_default_delegate=CreateDefaultDelegate(_generator_default_delegate_funcz);

}

SQGlobalState::~SQGlobalState()
{
	while(!_systemstrings->empty()){
		_systemstrings->back()=_null_;
		_systemstrings->pop_back();
	}
	_table_default_delegate=_null_;
	_array_default_delegate=_null_;
	_string_default_delegate=_null_;
	_number_default_delegate=_null_;
	_closure_default_delegate=_null_;
	_generator_default_delegate=_null_;
	//make sure that all strings are deleted
	
#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
	SQCollectable *v=_vms_chain;
	while(_vms_chain){
		v->Release();
		v=_vms_chain;
	}

	SQCollectable *t=_gc_chain;
	while(t){
		t->_uiRef++;
		t->Clear();
		if(--t->_uiRef==0)t->Release();
		if(t!=_gc_chain)
			t=_gc_chain;
		else
			t=t->_next;
	}
	assert(_gc_chain==NULL); //just to proove a theory
	while(_gc_chain){
		_gc_chain->_uiRef++;
		_gc_chain->Release();
	}
#endif
	sq_delete(_types,SQObjectPtrVec);
	sq_delete(_systemstrings,SQObjectPtrVec);
	sq_delete(_metamethods,SQObjectPtrVec);
	sq_delete(_stringtable,StringTable);
	if(_scratchpad)SQ_FREE(_scratchpad,_scratchpadsize);
}

#ifdef GARBAGE_COLLECTOR

void SQGlobalState::MarkObject(SQObjectPtr &o,SQCollectable **chain)
{
	switch(type(o)){
	case OT_TABLE:_table(o)->Mark(chain);break;
	case OT_ARRAY:_array(o)->Mark(chain);break;
	case OT_USERDATA:_userdata(o)->Mark(chain);break;
	case OT_CLOSURE:_closure(o)->Mark(chain);break;
	case OT_NATIVECLOSURE:_nativeclosure(o)->Mark(chain);break;
	case OT_GENERATOR:_generator(o)->Mark(chain);break;
	}
}

void SQVM::Mark(SQCollectable **chain)
{
	SQGlobalState::MarkObject(_roottable,chain);
	SQGlobalState::MarkObject(_lasterror,chain);
	SQGlobalState::MarkObject(_errorhandler,chain);
	SQGlobalState::MarkObject(_debughook,chain);
	SQGlobalState::MarkObject(_refs_table,chain);
	SQGlobalState::MarkObject(temp,chain);
	for(unsigned int i=0;i<_stack.size();i++)SQGlobalState::MarkObject(_stack[i],chain);
}

int SQGlobalState::CollectGarbage(SQVM *vm)
{
	int n=0;
	SQCollectable *tchain=NULL;
	SQCollectable *vms=_vms_chain;
	
	while(vms){
		vms->Mark(&tchain);
		vms=vms->_next;
	}

	MarkObject(_table_default_delegate,&tchain);
	MarkObject(_array_default_delegate,&tchain);
	MarkObject(_string_default_delegate,&tchain);
	MarkObject(_number_default_delegate,&tchain);
	MarkObject(_generator_default_delegate,&tchain);
	MarkObject(_closure_default_delegate,&tchain);

	SQCollectable *t=_gc_chain;
	while(t){
		t->_uiRef++;
		t->Clear();
		if(--t->_uiRef==0)t->Release();
		if(t!=_gc_chain)
			t=_gc_chain;
		else
			t=t->_next;
		n++;
	}

	t=tchain;
	while(t){
		t->UnMark();
		t=t->_next;
	}
	_gc_chain=tchain;
	return n;
}
#endif

#if defined(CYCLIC_REF_SAFE) || defined(GARBAGE_COLLECTOR)
void SQCollectable::AddToChain(SQCollectable **chain,SQCollectable *c)
{
    c->_prev=NULL;
	c->_next=*chain;
	if(*chain) (*chain)->_prev=c;
	*chain=c;
}

void SQCollectable::RemoveFromChain(SQCollectable **chain,SQCollectable *c)
{
	if(c->_prev) c->_prev->_next=c->_next;
	else *chain=c->_next;
	if(c->_next)
		c->_next->_prev=c->_prev;
	c->_next=NULL;
	c->_prev=NULL;
}

#endif

SQChar* SQGlobalState::GetScratchPad(int size)
{
	int newsize;
	if(size>0){
		if(_scratchpadsize<size){
			newsize=size+(size>>1);
			_scratchpad=(SQChar *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize=newsize;

		}else if(_scratchpadsize>=(size<<5)){
			newsize=_scratchpadsize>>1;
			_scratchpad=(SQChar *)SQ_REALLOC(_scratchpad,_scratchpadsize,newsize);
			_scratchpadsize=newsize;
		}
	}
	return _scratchpad;
}

//////////////////////////////////////////////////////////////////////////
//StringTable
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_lstring.c.html
*/


StringTable::StringTable()
{
	AllocNodes(4);
}

StringTable::~StringTable()
{
	SQ_FREE(_strings,sizeof(SQString*)*_numofslots);
	_strings=NULL;
}

void StringTable::AllocNodes(int size)
{
	_numofslots=size;
	_slotused=0;
	_strings=(SQString**)SQ_MALLOC(sizeof(SQString*)*_numofslots);
	memset(_strings,0,sizeof(SQString*)*_numofslots);
}

SQString *StringTable::Add(const SQChar *news,int len)
{
	if(len<0)
		len=strlen(news);
	unsigned int h=::_hashstr(news,len)&(_numofslots-1);
	SQString *s;
	for (s = _strings[h]; s; s = s->_next){
		if(s->_len==len && (!memcmp(news,s->_val,len)))
			return s; //found
	}
	SQString *t=(SQString *)SQ_MALLOC(len+sizeof(SQString));
	new (t) SQString;
	memcpy(t->_val,news,len);
	t->_val[len]='\0';
	t->_len=len;
	t->_hash=::_hashstr(news,len);
	t->_next=_strings[h];
	t->_uiRef=0;
	_strings[h]=t;
	_slotused++;
	if (_slotused > _numofslots)  /* too crowded? */
		Resize(_numofslots*2);
	return t;
}

void StringTable::Resize(int size)
{
	int oldsize=_numofslots;
	SQString **oldtable=_strings;
	AllocNodes(size);
	for (int i=0; i<oldsize; i++){
		SQString *p = oldtable[i];
		while(p){
			SQString *next = p->_next;
			unsigned int h=p->_hash&(_numofslots-1);
			p->_next=_strings[h];
			_strings[h] = p;
			p=next;
		}
	}
	SQ_FREE(oldtable,oldsize*sizeof(SQString*));
}

void StringTable::Remove(SQString *bs)
{
	SQString *s;
	SQString *prev=NULL;
	unsigned int h=bs->_hash&(_numofslots-1);
	for (s = _strings[h]; s; ){
		if(s->_len==bs->_len && (!memcmp(bs->_val,s->_val,bs->_len))){
			if(prev)
				prev->_next=s->_next;
			else
				_strings[h]=s->_next;
			_slotused--;
			int slen=s->_len;
			s->~SQString();
			SQ_FREE(s,sizeof(SQString)+slen);
			return;
		}
		prev = s;
		s = s->_next;
	}
	//if this fail something is wrong
	assert(0);
}


