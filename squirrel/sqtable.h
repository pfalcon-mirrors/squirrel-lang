/*	see copyright notice in squirrel.h */
#ifndef _SQTABLE_H_
#define _SQTABLE_H_
/*
* The following code is based on Lua 4.0 (Copyright 1994-2002 Tecgraf, PUC-Rio.)
* http://www.lua.org/copyright.html#4
* http://www.lua.org/source/4.0.1/src_ltable.c.html
*/

#include "sqstring.h"

#define hashptr(p)  (((unsigned long)(p)) >> 3)

struct SQTable : public CHAINABLE_OBJ 
{
private:
	struct _HashNode
	{
		SQObjectPtr val;
		SQObjectPtr key;
		_HashNode *next;
	};
	_HashNode *_firstfree;
	_HashNode *_nodes;
	int _numofnodes;
	
///////////////////////////
	void AllocNodes(int nSize);
	
	void Rehash();
	SQTable(SQSharedState *ss,int nInitialSize)
	{
		int pow2size=MINPOWER2;
		while(nInitialSize>pow2size)pow2size=pow2size<<1;
		AllocNodes(pow2size);
		_uiRef=0;
		_delegate=NULL;
		INIT_CHAIN();
		ADD_TO_CHAIN(&_sharedstate->_gc_chain,this);
	}
public:
	SQTable *_delegate;
	static SQTable* Create(SQSharedState *ss,int nInitialSize)
	{
		SQTable *newtable=(SQTable*)SQ_MALLOC(sizeof(SQTable));
		new (newtable) SQTable(ss,nInitialSize);
		newtable->_delegate=NULL;
		return newtable;
	}
	void Clear()
	{
		for(int i=0;i<_numofnodes;i++){_nodes[i].key=_null_;_nodes[i].val=_null_;}
	}
	SQTable *Clone();
	~SQTable()
	{
		SetDelegate(NULL);
		REMOVE_FROM_CHAIN(&_sharedstate->_gc_chain,this);
		for(int i=0;i<_numofnodes;i++)_nodes[i].~_HashNode();
		SQ_FREE(_nodes,_numofnodes*sizeof(_HashNode));
	}
#ifdef GARBAGE_COLLECTOR
	void Mark(SQCollectable **chain);
#endif
	inline unsigned long HashKey(const SQObjectPtr &key)
	{
		switch(type(key)){
			case OT_STRING:return _string(key)->_hash;
			case OT_FLOAT:return (unsigned long)((long)_float(key));
			case OT_INTEGER:return (unsigned long)((long)_integer(key));
			default:return hashptr(key._unVal.pRefCounted);
		}
	}
	inline _HashNode *_Get(const SQObjectPtr &key,unsigned long hash)
	{
		_HashNode *n=&_nodes[hash];
		SQObjectType ktype=type(key);
		do{
			if(type(n->key)==type(key) && _rawval(n->key)==_rawval(key)){
				return n;
			}
			n=n->next;
		}while(n);
		return NULL;
	}
	bool Get(const SQObjectPtr &key,SQObjectPtr &val)
	{
		_HashNode *n=_Get(key,HashKey(key)&(_numofnodes-1));
		if(n){
			val=n->val;
			return true;
		}
		return false;
	}
	void Remove(const SQObjectPtr &key)
	{
		unsigned long h=HashKey(key);//_string(key)->_hash;
		_HashNode *n=&_nodes[h&(_numofnodes-1)];
		_HashNode *first=n;
		_HashNode *prev=NULL;

		do
		{
			if(type(n->key)==type(key) && _rawval(n->key)==_rawval(key)){
				if(n==first && n->next){
					_nodes[h&(_numofnodes-1)].key=n->next->key;
					_nodes[h&(_numofnodes-1)].val=n->next->val;
					n->next->val=n->next->key=_null_;
				}
				else{
					if(prev)
						prev->next=n->next;
				}
				if(n>_firstfree)
					_firstfree=n;
				n->val=n->key=_null_;
			}
			prev=n;
			n=n->next;
		}while(n);
		
	}
	bool Set(const SQObjectPtr &key,const SQObjectPtr &val)
	{
		_HashNode *n=_Get(key,HashKey(key)&(_numofnodes-1));
		if(n){
			n->val=val;
			return true;
		}
		return false;
	}
	//returns true if a new slot has been created false if it was already present
	bool NewSlot(const SQObjectPtr &key,const SQObjectPtr &val)
	{
		unsigned long h=HashKey(key)&(_numofnodes-1);
		_HashNode *n=_Get(key,h);
		if(n){
			n->val=val;
			return false;
		}
		_HashNode *mp=&_nodes[h];
		n=mp;

		//key not found I'll insert it
		//main pos is not free

		if(type(mp->key)!=OT_NULL){
					
			_HashNode *othern;  /* main position of colliding node */
			n = _firstfree;  /* get a free place */
			if (mp > n && (othern=&_nodes[h]) != mp){
				/* yes; move colliding node into free position */
				while (othern->next != mp)
					othern = othern->next;  /* find previous */
				othern->next = n;  /* redo the chain with `n' in place of `mp' */
				*n = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
				mp->next = NULL;  /* now `mp' is free */
			}
			else{
				/* new node will go into free position */
				n->next = mp->next;  /* chain new position */
				mp->next = n;
				mp = n;
			}
		}
		mp->key = key;
		
		for (;;){  /* correct `firstfree' */
			if (type(_firstfree->key) == OT_NULL){
				mp->val=val;
				return true;  /* OK; table still has a free place */
			}
			else if (_firstfree == _nodes) break;  /* cannot decrement from here */
			else (_firstfree)--;
		}
		Rehash();
		return NewSlot(key,val);
	}
	int Next(const SQObjectPtr &refpos,SQObjectPtr &outkey,SQObjectPtr &outval)
	{
		//first iteration
		int idx;
		switch(type(refpos)){
		case OT_NULL:
			idx=0;
			break;
		case OT_INTEGER:
			idx=(int)_integer(refpos);
			break;
		default:
			////sqraiseerror("critical vm error iterating a table with a non number idx");
			assert(0);
			break;
		}
		
		while(idx<_numofnodes){
			if(type(_nodes[idx].key)!=OT_NULL){
				//first found
				outkey=_nodes[idx].key;
				outval=_nodes[idx].val;
				//return idx for the next iteration
				return ++idx;
			}
			++idx;
		}
		//nothing to iterate anymore
		return -1;
	}
	int CountUsed();
	void Release()
	{
		sq_delete(this,SQTable);
	}
	void SetDelegate(SQTable *mt)
	{
		if(mt)
			mt->_uiRef++;
		if(_delegate){
			_delegate->_uiRef--;
			if(_delegate->_uiRef==0)
				_delegate->Release();
		}
		_delegate=mt;
	}
};

SQTable *Table_CreateUnifiedMethods();

#endif //_SQTABLE_H_
